/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QAbstractTextDocumentLayout>
# include <QApplication>
# include <QClipboard>
# include <QDateTime>
# include <QHBoxLayout>
# include <QMessageBox>
# include <QPainter>
# include <QPrinter>
# include <QPrintDialog>
# include <QScrollBar>
# include <QPlainTextEdit>
# include <QPrintPreviewDialog>
# include <QTextBlock>
# include <QTextCodec>
# include <QTextStream>
# include <QTimer>
#endif

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QShortcut>

#include "EditorView.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "FileDialog.h"
#include "Macro.h"
#include "PythonDebugger.h"
#include "PythonEditor.h"

#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>

using namespace Gui;
namespace Gui {
class EditorViewP {
public:
    QPlainTextEdit* textEdit;
    EditorSearchBar* searchBar;
    QString fileName;
    EditorView::DisplayName displayName;
    QTimer*  activityTimer;
    uint timeStamp;
    bool lock;
    QStringList undos;
    QStringList redos;
};
}


// -------------------------------------------------------

/* TRANSLATOR Gui::EditorView */

/**
 *  Constructs a EditorView which is a child of 'parent', with the
 *  name 'name'.
 */
EditorView::EditorView(QPlainTextEdit* editor, QWidget* parent)
    : MDIView(0,parent,0), WindowParameter( "Editor" )
{
    d = new EditorViewP;
    d->lock = false;
    d->displayName = EditorView::FullName;

    // create the editor first
    d->textEdit = editor;
    d->textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    // create searchbar
    d->searchBar = new EditorSearchBar(this, d);

    // Create the layout containing the workspace and a tab bar
    QVBoxLayout* vLayout = new QVBoxLayout;
    QFrame*      vbox  = new QFrame(this);
    vLayout->setMargin(0);
    vLayout->addWidget(d->textEdit);
    vLayout->addWidget(d->searchBar);
    vbox->setLayout(vLayout);

    QFrame* hbox = new QFrame(this);
    hbox->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->setMargin(1);
    hLayout->addWidget(vbox);
    d->textEdit->setParent(vbox);
    hbox->setLayout(hLayout);
    setCentralWidget(hbox);

    setCurrentFileName(QString());
    d->textEdit->setFocus();

    setWindowIcon(d->textEdit->windowIcon());

    ParameterGrp::handle hPrefGrp = getWindowParameter();
    hPrefGrp->Attach( this );
    hPrefGrp->NotifyAll();

    d->activityTimer = new QTimer(this);
    connect(d->activityTimer, SIGNAL(timeout()),
            this, SLOT(checkTimestamp()) );
    connect(d->textEdit->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(setWindowModified(bool)));
    connect(d->textEdit->document(), SIGNAL(undoAvailable(bool)),
            this, SLOT(undoAvailable(bool)));
    connect(d->textEdit->document(), SIGNAL(redoAvailable(bool)),
            this, SLOT(redoAvailable(bool)));
    connect(d->textEdit->document(), SIGNAL(contentsChange(int, int, int)),
            this, SLOT(contentsChange(int, int, int)));

    // is set globaly
    //QShortcut* find = new QShortcut(this);
    //find->setKey(Qt::CTRL + Qt::Key_F );
    //connect(find, SIGNAL(activated()), d->searchBar, SLOT(show()));
}

/** Destroys the object and frees any allocated resources */
EditorView::~EditorView()
{
    d->activityTimer->stop();
    delete d->activityTimer;
    delete d;
    getWindowParameter()->Detach( this );
}

QPlainTextEdit* EditorView::getEditor() const
{
    return d->textEdit;
}

void EditorView::OnChange(Base::Subject<const char*> &rCaller,const char* rcReason)
{
    Q_UNUSED(rCaller); 
    ParameterGrp::handle hPrefGrp = getWindowParameter();
    if (strcmp(rcReason, "EnableLineNumber") == 0) {
        //bool show = hPrefGrp->GetBool( "EnableLineNumber", true );
    }
}

void EditorView::checkTimestamp()
{
    QFileInfo fi(d->fileName);
    uint timeStamp =  fi.lastModified().toTime_t();
    if (timeStamp != d->timeStamp) {
        switch( QMessageBox::question( this, tr("Modified file"), 
                tr("%1.\n\nThis has been modified outside of the source editor. Do you want to reload it?").arg(d->fileName),
                QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape) )
        {
            case QMessageBox::Yes:
                // updates time stamp and timer
                open( d->fileName );
                return;
            case QMessageBox::No:
                d->timeStamp = timeStamp;
                break;
        }
    }

    d->activityTimer->setSingleShot(true);
    d->activityTimer->start(3000);
}

/**
 * Runs the action specified by \a pMsg.
 */
bool EditorView::onMsg(const char* pMsg,const char** /*ppReturn*/)
{
    if (strcmp(pMsg,"Save")==0){
        saveFile();
        return true;
    } else if (strcmp(pMsg,"SaveAs")==0){
        saveAs();
        return true;
    } else if (strcmp(pMsg,"Cut")==0){
        cut();
        return true;
    } else if (strcmp(pMsg,"Copy")==0){
        copy();
        return true;
    } else if (strcmp(pMsg,"Paste")==0){
        paste();
        return true;
    } else if (strcmp(pMsg,"Undo")==0){
        undo();
        return true;
    } else if (strcmp(pMsg,"Redo")==0){
        redo();
        return true;
    } else if (strcmp(pMsg,"ViewFit")==0){
        // just ignore this
        return true;
    } else if (strcmp(pMsg,"ShowFindBar")==0){
        showFindBar();
        return true;
    } else if (strcmp(pMsg,"HideFindBar")==0){
        hideFindBar();
        return true;
    }

    return false;
}

/**
 * Checks if the action \a pMsg is available. This is for enabling/disabling
 * the corresponding buttons or menu items for this action.
 */
bool EditorView::onHasMsg(const char* pMsg) const
{
    if (strcmp(pMsg,"Run")==0)  return true;
    if (strcmp(pMsg,"DebugStart")==0)  return true;
    if (strcmp(pMsg,"DebugStop")==0)  return true;
    if (strcmp(pMsg,"SaveAs")==0)  return true;
    if (strcmp(pMsg,"Print")==0) return true;
    if (strcmp(pMsg,"PrintPreview")==0) return true;
    if (strcmp(pMsg,"PrintPdf")==0) return true;
    if (strcmp(pMsg,"Save")==0) { 
        return d->textEdit->document()->isModified();
    } else if (strcmp(pMsg,"Cut")==0) {
        bool canWrite = !d->textEdit->isReadOnly();
        return (canWrite && (d->textEdit->textCursor().hasSelection()));
    } else if (strcmp(pMsg,"Copy")==0) {
        return ( d->textEdit->textCursor().hasSelection() );
    } else if (strcmp(pMsg,"Paste")==0) {
        QClipboard *cb = QApplication::clipboard();
        QString text;

        // Copy text from the clipboard (paste)
        text = cb->text();

        bool canWrite = !d->textEdit->isReadOnly();
        return ( !text.isEmpty() && canWrite );
    } else if (strcmp(pMsg,"Undo")==0) {
        return d->textEdit->document()->isUndoAvailable ();
    } else if (strcmp(pMsg,"Redo")==0) {
        return d->textEdit->document()->isRedoAvailable ();
    } else if (strcmp(pMsg,"ShowFindBar")==0) {
        return d->searchBar->isHidden();
    } else if (strcmp(pMsg,"HideFindBar")==0) {
        return !d->searchBar->isHidden();;
    }

    return false;
}

/** Checking on close state. */
bool EditorView::canClose(void)
{
    if ( !d->textEdit->document()->isModified() )
        return true;
    this->setFocus(); // raises the view to front
    switch( QMessageBox::question(this, tr("Unsaved document"), 
                                    tr("The document has been modified.\n"
                                       "Do you want to save your changes?"),
                                     QMessageBox::Yes|QMessageBox::Default, QMessageBox::No, 
                                     QMessageBox::Cancel|QMessageBox::Escape))
    {
        case QMessageBox::Yes:
            return saveFile();
        case QMessageBox::No:
            return true;
        case QMessageBox::Cancel:
            return false;
        default:
            return false;
    }
}

void EditorView::setDisplayName(EditorView::DisplayName type)
{
    d->displayName = type;
}

/**
 * Saves the content of the editor to a file specified by the appearing file dialog.
 */
bool EditorView::saveAs(void)
{
    QString fn = FileDialog::getSaveFileName(this, QObject::tr("Save Macro"),
        QString::null, QString::fromLatin1("%1 (*.FCMacro);;Python (*.py)").arg(tr("FreeCAD macro")));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return saveFile();
}

/**
 * Opens the file \a fileName.
 */
bool EditorView::open(const QString& fileName)
{
    if (!QFile::exists(fileName))
        return false;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return false;

    d->lock = true;
    d->textEdit->setPlainText(QString::fromUtf8(file.readAll()));
    d->lock = false;
    d->undos.clear();
    d->redos.clear();
    file.close();

    QFileInfo fi(fileName);
    d->timeStamp =  fi.lastModified().toTime_t();
    d->activityTimer->setSingleShot(true);
    d->activityTimer->start(3000);

    setCurrentFileName(fileName);
    return true;
}

/**
 * Copies the selected text to the clipboard and deletes it from the text edit.
 * If there is no selected text nothing happens.
 */
void EditorView::cut(void)
{
    d->textEdit->cut();
}

/**
 * Copies any selected text to the clipboard.
 */
void EditorView::copy(void)
{
    d->textEdit->copy();
}

/**
 * Pastes the text from the clipboard into the text edit at the current cursor position. 
 * If there is no text in the clipboard nothing happens.
 */
void EditorView::paste(void)
{
    d->textEdit->paste();
}

/**
 * Undoes the last operation.
 * If there is no operation to undo, i.e. there is no undo step in the undo/redo history, nothing happens.
 */
void EditorView::undo(void)
{
    d->lock = true;
    if (!d->undos.isEmpty()) {
        d->redos << d->undos.back();
        d->undos.pop_back();
    }
    d->textEdit->document()->undo();
    d->lock = false;
}

/**
 * Redoes the last operation.
 * If there is no operation to undo, i.e. there is no undo step in the undo/redo history, nothing happens.
 */
void EditorView::redo(void)
{
    d->lock = true;
    if (!d->redos.isEmpty()) {
        d->undos << d->redos.back();
        d->redos.pop_back();
    }
    d->textEdit->document()->redo();
    d->lock = false;
}

/**
 * Shows the printer dialog.
 */
void EditorView::print()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        d->textEdit->document()->print(&printer);
    }
}

void EditorView::printPreview()
{
    QPrinter printer(QPrinter::ScreenResolution);
    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, SIGNAL(paintRequested (QPrinter *)),
            this, SLOT(print(QPrinter *)));
    dlg.exec();
}

void EditorView::print(QPrinter* printer)
{
    d->textEdit->document()->print(printer);
}

void EditorView::showFindBar()
{
    d->searchBar->show();
}

void EditorView::hideFindBar()
{
    d->searchBar->hide();
}

/**
 * Prints the document into a Pdf file.
 */
void EditorView::printPdf()
{
    QString filename = FileDialog::getSaveFileName(this, tr("Export PDF"), QString(),
        QString::fromLatin1("%1 (*.pdf)").arg(tr("PDF file")));
    if (!filename.isEmpty()) {
        QPrinter printer(QPrinter::ScreenResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        d->textEdit->document()->print(&printer);
    }
}

void EditorView::setCurrentFileName(const QString &fileName)
{
    d->fileName = fileName;
    /*emit*/ changeFileName(d->fileName);
    d->textEdit->document()->setModified(false);

    QString name;
    QFileInfo fi(fileName);
    switch (d->displayName) {
    case FullName:
        name = fileName;
        break;
    case FileName:
        name = fi.fileName();
        break;
    case BaseName:
        name = fi.baseName();
        break;
    }

    QString shownName;
    if (fileName.isEmpty())
        shownName = tr("untitled[*]");
    else
        shownName = QString::fromLatin1("%1[*]").arg(name);
    shownName += tr(" - Editor");
    setWindowTitle(shownName);
    setWindowModified(false);
}

QString EditorView::fileName() const
{
    return d->fileName;
}

/**
 * Saves the contents to a file.
 */
bool EditorView::saveFile()
{
    if (d->fileName.isEmpty())
        return saveAs();

    QFile file(d->fileName);
    if (!file.open(QFile::WriteOnly))
        return false;


    // trim trailing whitespace?
    ParameterGrp::handle hPrefGrp = getWindowParameter();
    if (hPrefGrp->GetBool("EnableTrimTrailingWhitespaces", true )) {
            // to restore cursor and scroll position
            QTextCursor cursor = d->textEdit->textCursor();
            int oldPos = cursor.position(),
                vScroll = d->textEdit->verticalScrollBar()->value(),
                hScroll = d->textEdit->horizontalScrollBar()->value();

            QStringList rows = d->textEdit->document()->toPlainText()
                                    .split(QLatin1Char('\n'));
            int delCount = 0, chPos = -1;
            for (QString &row : rows) {
                ++chPos; // for the newline
                int i =  row.size();
                while(i > 0 && row[i - 1].isSpace())
                    --i;
                if (chPos + row.size() - i <= oldPos) // for restore cursorposition
                    delCount += row.size() - i;
                chPos += row.size();
                row.remove(i, row.size() - i);
            }

            QString txt = rows.join(QLatin1String("\n"));
            d->textEdit->document()->setPlainText(txt);

            // restore cursor and scroll position
            d->textEdit->verticalScrollBar()->setValue(vScroll);
            d->textEdit->horizontalScrollBar()->setValue(hScroll);
            cursor.setPosition(oldPos - delCount);
            d->textEdit->setTextCursor(cursor);
    }

    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << d->textEdit->document()->toPlainText();
    file.close();

    d->textEdit->document()->setModified(false);

    QFileInfo fi(d->fileName);
    d->timeStamp =  fi.lastModified().toTime_t();
    return true;
}

void EditorView::undoAvailable(bool undo)
{
    if (!undo)
        d->undos.clear();
}

void EditorView::redoAvailable(bool redo)
{
    if (!redo)
        d->redos.clear();
}

void EditorView::contentsChange(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(position); 
    if (d->lock)
        return;
    if (charsRemoved > 0 && charsAdded > 0)
        return; // syntax highlighting
    else if (charsRemoved > 0)
        d->undos << tr("%1 chars removed").arg(charsRemoved);
    else if (charsAdded > 0)
        d->undos << tr("%1 chars added").arg(charsAdded);
    else
        d->undos << tr("Formatted");
    d->redos.clear();
}

/**
 * Get the undo history.
 */
QStringList EditorView::undoActions() const
{
    return d->undos;
}

/**
 * Get the redo history.
 */
QStringList EditorView::redoActions() const
{
    return d->redos;;
}

void EditorView::focusInEvent (QFocusEvent *)
{
    d->textEdit->setFocus();
}

// ---------------------------------------------------------

PythonEditorView::PythonEditorView(PythonEditor* editor, QWidget* parent)
  : EditorView(editor, parent), _pye(editor)
{
    connect(this, SIGNAL(changeFileName(const QString&)),
            editor, SLOT(setFileName(const QString&)));
}

PythonEditorView::~PythonEditorView()
{
}

/**
 * Runs the action specified by \a pMsg.
 */
bool PythonEditorView::onMsg(const char* pMsg,const char** ppReturn)
{
    if (strcmp(pMsg,"Run")==0) {
        executeScript();
        return true;
    }
    else if (strcmp(pMsg,"StartDebug")==0) {
        QTimer::singleShot(300, this, SLOT(startDebug()));
        return true;
    }
    else if (strcmp(pMsg,"ToggleBreakpoint")==0) {
        toggleBreakpoint();
        return true;
    }
    return EditorView::onMsg(pMsg, ppReturn);
}

/**
 * Checks if the action \a pMsg is available. This is for enabling/disabling
 * the corresponding buttons or menu items for this action.
 */
bool PythonEditorView::onHasMsg(const char* pMsg) const
{
    if (strcmp(pMsg,"Run")==0)  return true;
    if (strcmp(pMsg,"StartDebug")==0)  return true;
    if (strcmp(pMsg,"ToggleBreakpoint")==0)  return true;
    return EditorView::onHasMsg(pMsg);
}

/**
 * Runs the opened script in the macro manager.
 */
void PythonEditorView::executeScript()
{
    // always save the macro when it is modified
    if (EditorView::onHasMsg("Save"))
        EditorView::onMsg("Save", 0);
    try {
        Application::Instance->macroManager()->run(Gui::MacroManager::File,fileName().toUtf8());
    }
    catch (const Base::SystemExitException&) {
        // handle SystemExit exceptions
        Base::PyGILStateLocker locker;
        Base::PyException e;
        e.ReportException();
    }
}

void PythonEditorView::startDebug()
{
    _pye->startDebug();
}

void PythonEditorView::toggleBreakpoint()
{
    _pye->toggleBreakpoint();
}

void PythonEditorView::showDebugMarker(int line)
{
    _pye->showDebugMarker(line);
}

void PythonEditorView::hideDebugMarker()
{
    _pye->hideDebugMarker();
}

// -------------------------------------------------------------------------------------


EditorSearchBar::EditorSearchBar(EditorView *parent, EditorViewP *editorViewP) :
    QFrame(parent),
    d(editorViewP)
{
    // find row
    QGridLayout *layout  = new QGridLayout(this);
    QLabel      *lblFind = new QLabel(this);
    m_foundCountLabel    = new QLabel(this);
    m_searchEdit         = new QLineEdit(this);
    m_searchButton       = new QPushButton(this);
    m_upButton           = new QPushButton(this);
    m_downButton         = new QPushButton(this);
    m_hideButton         = new QPushButton(this);


    lblFind->setText(tr("Find:"));
    m_searchButton->setText(tr("Search"));
    m_upButton->setIcon(BitmapFactory().iconFromTheme("button_left"));
    m_downButton->setIcon(BitmapFactory().iconFromTheme("button_right"));
    m_searchEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_foundCountLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_hideButton->setIcon(BitmapFactory().iconFromTheme("delete"));

    layout->addWidget(lblFind, 0, 0);
    layout->addWidget(m_searchEdit, 0, 1);
    layout->addWidget(m_searchButton, 0, 2);
    layout->addWidget(m_foundCountLabel, 0, 3);
    layout->addWidget(m_upButton, 0, 4);
    layout->addWidget(m_downButton, 0, 5);
    layout->addWidget(m_hideButton, 0, 6);

    // replace row
    QLabel *lblReplace     = new QLabel(this);
    m_replaceEdit          = new QLineEdit(this);
    m_replaceButton        = new QPushButton(this);
    m_replaceAndNextButton = new QPushButton(this);
    m_replaceAllButton     = new QPushButton(this);
    QHBoxLayout *buttonBox = new QHBoxLayout;

    lblReplace->setText(tr("Replace:"));
    m_replaceEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addWidget(lblReplace, 1, 0);
    layout->addWidget(m_replaceEdit, 1, 1);

    m_replaceButton->setText(tr("Replace"));
    m_replaceAndNextButton->setText(tr("Replace & find"));
    m_replaceAllButton->setText(tr("Replace All"));
    buttonBox->addWidget(m_replaceButton);
    buttonBox->addWidget(m_replaceAndNextButton);
    buttonBox->addWidget(m_replaceAllButton);
    layout->addLayout(buttonBox, 1, 2, 1, 5);

    setLayout(layout);

    connect(m_hideButton, SIGNAL(clicked()), this, SLOT(hide()));
    connect(m_upButton, SIGNAL(clicked()), this, SLOT(upSearch()));
    connect(m_downButton, SIGNAL(clicked()), this, SLOT(downSearch()));
    connect(m_searchButton, SIGNAL(clicked()), this, SLOT(downSearch()));
    connect(m_searchEdit, SIGNAL(returnPressed()), this, SLOT(downSearch()));
    connect(m_searchEdit, SIGNAL(textChanged(QString)),
                        this, SLOT(searchChanged(QString)));

    connect(m_replaceButton, SIGNAL(clicked()), this, SLOT(replace()));
    connect(m_replaceAndNextButton, SIGNAL(clicked()),
                        this, SLOT(replaceAndFind()));
    connect(m_replaceAllButton, SIGNAL(clicked()),
                        this, SLOT(replaceAll()));


    hide();
}

EditorSearchBar::~EditorSearchBar()
{
}

void EditorSearchBar::show()
{
    QFrame::show();
    m_searchEdit->setFocus();
}

void EditorSearchBar::upSearch(bool cycle)
{
    if (m_searchEdit->text().size()) {
        if (!d->textEdit->find(m_searchEdit->text(), QTextDocument::FindBackward)
            && cycle)
        {
            // start over
            QTextCursor cursor = d->textEdit->textCursor();
            if (!cursor.isNull()) {
                cursor.movePosition(QTextCursor::End);
                d->textEdit->setTextCursor(cursor);
            }
            return upSearch(false);
        }
    }
    d->textEdit->repaint();
    m_searchEdit->setFocus();
}

void EditorSearchBar::downSearch(bool cycle)
{
    if (m_searchEdit->text().size()) {
        if (!d->textEdit->find(m_searchEdit->text()) && cycle) {
            // start over
            QTextCursor cursor = d->textEdit->textCursor();
            if (!cursor.isNull()) {
                cursor.movePosition(QTextCursor::Start);
                d->textEdit->setTextCursor(cursor);
            }
            return downSearch(false);
        }
    }
    d->textEdit->repaint();
    m_searchEdit->setFocus();
}

void EditorSearchBar::foundCount(int foundOcurrences)
{
    QString found = QString::number(foundOcurrences);
    m_foundCountLabel->setText(tr("Found: %1 occurences").arg(found));
}

void EditorSearchBar::searchChanged(const QString &str)
{
    int found = static_cast<TextEditor*>(d->textEdit)->findAndHighlight(str);
    d->textEdit->repaint();
    foundCount(found);
}

void EditorSearchBar::replace()
{
    if (!m_replaceEdit->text().size() || !m_searchEdit->text().size())
        return;

    QTextCursor cursor = d->textEdit->textCursor();
    if (cursor.hasSelection()) {
        cursor.insertText(m_replaceEdit->text());
        searchChanged(m_searchEdit->text());
    }
}

void EditorSearchBar::replaceAndFind()
{
    if (!m_replaceEdit->text().size() || !m_searchEdit->text().size())
        return;

    replace();
    d->textEdit->find(m_searchEdit->text());
}

void EditorSearchBar::replaceAll()
{
    if (!m_replaceEdit->text().size() || !m_searchEdit->text().size())
        return;

    QTextCursor cursor = d->textEdit->textCursor();
    int oldPos = cursor.position();

    cursor = d->textEdit->document()->find(m_searchEdit->text());

    while (!cursor.isNull()) {
        cursor.insertText(m_replaceEdit->text());
        cursor = d->textEdit->document()->find(m_searchEdit->text(), cursor);
    }

    searchChanged(m_searchEdit->text());

    cursor.setPosition(oldPos);
    d->textEdit->setTextCursor(cursor);
}

#include "moc_EditorView.cpp"
