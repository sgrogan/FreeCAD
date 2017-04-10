/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QContextMenuEvent>
# include <QMenu>
# include <QPainter>
# include <QShortcut>
# include <QTextCursor>
#endif

#include "PythonEditor.h"
#include "PythonDebugger.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Macro.h"
#include "FileDialog.h"
#include "DlgEditorImp.h"
#include "CallTips.h"


#include <CXX/Objects.hxx>
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <QRegExp>
#include <QDebug>

using namespace Gui;

namespace Gui {
struct PythonEditorP
{
    int   debugLine;
    QRect debugRect;
    QPixmap breakpoint;
    QPixmap debugMarker;
    QString filename;
    PythonDebugger* debugger;
    CallTipsList* callTipsList;
    PythonMatchingChars* matchingChars;
    PythonEditorP()
        : debugLine(-1),
          breakpoint(BitmapFactory().iconFromTheme("breakpoint").pixmap(16,16)),
          debugMarker(BitmapFactory().iconFromTheme("debug-marker").pixmap(16,16)),
          callTipsList(0)
    {
        debugger = Application::Instance->macroManager()->debugger();
    }
};

} // namespace Gui

/* TRANSLATOR Gui::PythonEditor */

/**
 *  Constructs a PythonEditor which is a child of 'parent' and does the
 *  syntax highlighting for the Python language. 
 */
PythonEditor::PythonEditor(QWidget* parent)
  : TextEditor(parent)
{
    d = new PythonEditorP();
    PythonSyntaxHighlighter *hl = new PythonSyntaxHighlighter(this);
    this->setSyntaxHighlighter(hl);


    // set acelerators
    QShortcut* comment = new QShortcut(this);
    comment->setKey(Qt::ALT + Qt::Key_C);

    QShortcut* uncomment = new QShortcut(this);
    uncomment->setKey(Qt::ALT + Qt::Key_U);

    QShortcut* autoIndent = new QShortcut(this);
    autoIndent->setKey(Qt::CTRL + Qt::SHIFT + Qt::Key_I );

    connect(comment, SIGNAL(activated()), 
            this, SLOT(onComment()));
    connect(uncomment, SIGNAL(activated()), 
            this, SLOT(onUncomment()));
    connect(autoIndent, SIGNAL(activated()),
            this, SLOT(onAutoIndent()));

    d->matchingChars = new PythonMatchingChars(this);

    // create the window for call tips
    d->callTipsList = new CallTipsList(this); //, QLatin1String("__scriptcompletion__"));
    d->callTipsList->setFrameStyle(QFrame::Box|QFrame::Raised);
    d->callTipsList->setLineWidth(2);
    installEventFilter(d->callTipsList);
    viewport()->installEventFilter(d->callTipsList);
    d->callTipsList->setSelectionMode( QAbstractItemView::SingleSelection );
    d->callTipsList->hide();
}

/** Destroys the object and frees any allocated resources */
PythonEditor::~PythonEditor()
{
    getWindowParameter()->Detach( this );
    delete d;
}

void PythonEditor::setFileName(const QString& fn)
{
    d->filename = fn;
}

int PythonEditor::findText(const QString find)
{
    if (!find.size())
        return 0;

    QTextCharFormat format;
    format.setForeground(QColor(QLatin1String("#110059")));
    format.setBackground(QColor(QLatin1String("#fff356")));

    int found = 0;
    for (QTextBlock block = document()->begin();
         block.isValid(); block = block.next())
    {
        int pos = block.text().indexOf(find);
        if (pos > -1) {
            ++found;
            highlightText(block.position() + pos, find.size(), format);
        }
    }

    return found;
}

void PythonEditor::startDebug()
{
    if (d->debugger->start()) {
        d->debugger->runFile(d->filename);
        d->debugger->stop();
    }
}

void PythonEditor::toggleBreakpoint()
{
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber() + 1;
    d->debugger->toggleBreakpoint(line, d->filename);
    getMarker()->update();
}

void PythonEditor::showDebugMarker(int line)
{
    d->debugLine = line;
    getMarker()->update();
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    int cur = cursor.blockNumber() + 1;
    if (cur > line) {
        for (int i=line; i<cur; i++)
            cursor.movePosition(QTextCursor::Up);
    }
    else if (cur < line) {
        for (int i=cur; i<line; i++)
            cursor.movePosition(QTextCursor::Down);
    }
    setTextCursor(cursor);
}

void PythonEditor::hideDebugMarker()
{
    d->debugLine = -1;
    getMarker()->update();
}

void PythonEditor::drawMarker(int line, int x, int y, QPainter* p)
{
    Breakpoint bp = d->debugger->getBreakpoint(d->filename);
    if (bp.checkLine(line)) {
        p->drawPixmap(x, y, d->breakpoint);
    }
    if (d->debugLine == line) {
        p->drawPixmap(x, y+2, d->debugMarker);
        d->debugRect = QRect(x, y+2, d->debugMarker.width(), d->debugMarker.height());
    }
}

void PythonEditor::contextMenuEvent ( QContextMenuEvent * e )
{
    QMenu* menu = createStandardContextMenu();
    if (!isReadOnly()) {
        menu->addSeparator();
        menu->addAction( tr("Comment"), this, SLOT( onComment() ), Qt::ALT + Qt::Key_C );
        menu->addAction( tr("Uncomment"), this, SLOT( onUncomment() ), Qt::ALT + Qt::Key_U );
        menu->addAction( tr("Auto indent"), this, SLOT( onAutoIndent() ), Qt::CTRL + Qt::SHIFT + Qt::Key_I );
    }

    menu->exec(e->globalPos());
    delete menu;
}

/**
 * Checks the input to make the correct indentations.
 * And code completions
 */
void PythonEditor::keyPressEvent(QKeyEvent * e)
{
    QTextCursor cursor = this->textCursor();
    QTextCursor inputLineBegin = this->textCursor();
    inputLineBegin.movePosition( QTextCursor::StartOfLine );
    static bool autoIndented = false;

    /**
    * The cursor sits somewhere on the input line (after the prompt)
    *   - restrict cursor movement to input line range (excluding the prompt characters)
    *   - roam the history by Up/Down keys
    *   - show call tips on period
    */
    QTextBlock inputBlock = inputLineBegin.block();              //< get the last paragraph's text
    QString    inputLine  = inputBlock.text();

    switch (e->key())
    {
    case Qt::Key_Backspace:
    {
        if (autoIndented) {
            cursor.beginEditBlock();
            int oldPos = cursor.position(),
                blockPos = cursor.block().position();
            // find previous block indentation
            QRegExp re(QLatin1String("[:{]\\s*$"));
            int pos = 0, ind = 0;
            while (cursor.movePosition(QTextCursor::Up,
                                       QTextCursor::KeepAnchor, 1))
            {
                if (re.indexIn(cursor.block().text()) > -1) {
                    // get leading indent of this row
                    QRegExp rx(QString::fromLatin1("^(\\s*)"));
                    pos = rx.indexIn(cursor.block().text());
                    ind = pos > -1 ? rx.cap(1).size() : -1;
                    if (blockPos + ind < oldPos)
                        break;
                }
                pos = 0;
                ind = 0;
            }

            if (ind > -1) {
                // reposition to start chr where we want to erase
                cursor.setPosition(blockPos + ind, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            }
            if (ind <= 0) {
                autoIndented = false;
            }

            cursor.endEditBlock();
        } else {
            TextEditor::keyPressEvent(e);
        }

    } break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    {
        // auto indent
        ParameterGrp::handle hPrefGrp = getWindowParameter();
        if (hPrefGrp->GetBool( "EnableAutoIndent", true)) {
            cursor.beginEditBlock();
            int indent = hPrefGrp->GetInt( "IndentSize", 4 );
            bool space = hPrefGrp->GetBool( "Spaces", false );
            QString ch = space ? QString(indent, QLatin1Char(' '))
                               : QString::fromLatin1("\t");

            // get leading indent
            QRegExp rx(QString::fromLatin1("^(\\s*)"));
            int pos = rx.indexIn(inputLine);
            QString ind = pos > -1 ? rx.cap(1) : QString();

            QRegExp re(QLatin1String("[:{]"));
            if (cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor) &&
                re.indexIn(cursor.selectedText()) > -1)
            {
                // a class or function
                ind += ch;
            }
            TextEditor::keyPressEvent(e);
            insertPlainText(ind);
            cursor.endEditBlock();
            autoIndented = true;
        } else {
            TextEditor::keyPressEvent(e);
        }
    }   break;

    case Qt::Key_Period:
    {
        autoIndented = false;

        // In Qt 4.8 there is a strange behaviour because when pressing ":"
        // then key is also set to 'Period' instead of 'Colon'. So we have
        // to make sure we only handle the period.
        if (e->text() == QLatin1String(".") && cursor != inputLineBegin) {
            // analyse context and show available call tips
            int contextLength = cursor.position() - inputLineBegin.position();
            TextEditor::keyPressEvent(e);
            d->callTipsList->showTips( inputLine.left( contextLength ) );
        }
        else {
            TextEditor::keyPressEvent(e);
        }
    }   break;
    default:
    {
        QString insertAfter;
        static QString previousKeyText;
        if (e->key() == Qt::Key_ParenLeft) {
            insertAfter = QLatin1String(")");
        } else if (e->key() == Qt::Key_BraceLeft) {
            insertAfter = QLatin1String("}");
        } else if (e->key() == Qt::Key_BracketLeft) {
            insertAfter = QLatin1String("]");
        } else if (e->key() == Qt::Key_QuoteDbl) {
            if (previousKeyText != e->text()) {
                insertAfter = QLatin1String("\"");
            } else { // last char was autoinserted, we wwant to step over
                QTextCursor cursor = textCursor();
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                setTextCursor(cursor);
            }
        } else if (e->key() == Qt::Key_Apostrophe) {
            if (previousKeyText != e->text()) {
                insertAfter = QLatin1String("'");
            } else { // last char was autoinserted, we wwant to step over
                QTextCursor cursor = textCursor();
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                setTextCursor(cursor);
            }
        }

        autoIndented = false;
        TextEditor::keyPressEvent(e);

        if (insertAfter.size()) {
            QTextCursor cursor = textCursor();
            cursor.insertText(insertAfter);
            cursor.movePosition(QTextCursor::Left);
            setTextCursor(cursor);
        }

        // need to use string instead of key as modifiers messes up otherwise
        if (e->text().size())
            previousKeyText = e->text();
    }   break;
    }

    // This can't be done in CallTipsList::eventFilter() because we must first perform
    // the event and afterwards update the list widget
    if (d->callTipsList->isVisible())
    { d->callTipsList->validateCursor(); }
}


void PythonEditor::onComment()
{
    QTextCursor cursor = textCursor();
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    QTextBlock block;
    cursor.beginEditBlock();
    for (block = document()->begin(); block.isValid(); block = block.next()) {
        int pos = block.position();
        int off = block.length()-1;
        // at least one char of the block is part of the selection
        if ( pos >= selStart || pos+off >= selStart) {
            if ( pos+1 > selEnd )
                break; // end of selection reached
            cursor.setPosition(block.position());
            cursor.insertText(QLatin1String("#"));
                selEnd++;
        }
    }

    cursor.endEditBlock();
}

void PythonEditor::onUncomment()
{
    QTextCursor cursor = textCursor();
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    QTextBlock block;
    cursor.beginEditBlock();
    for (block = document()->begin(); block.isValid(); block = block.next()) {
        int pos = block.position();
        int off = block.length()-1;
        // at least one char of the block is part of the selection
        if ( pos >= selStart || pos+off >= selStart) {
            if ( pos+1 > selEnd )
                break; // end of selection reached
            if (block.text().startsWith(QLatin1String("#"))) {
                cursor.setPosition(block.position());
                cursor.deleteChar();
                selEnd--;
            }
        }
    }

    cursor.endEditBlock();
}

void PythonEditor::onAutoIndent()
{
    QTextCursor cursor = textCursor();
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    QList<int> indentLevel;// level of indentation at different indentaions

    QTextBlock block;
    QRegExp reNewBlock(QLatin1String("[:{]\\s*#*.*$"));
    QChar mCommentChr(0);
    int mCommentIndents = 0;

    ParameterGrp::handle hPrefGrp = getWindowParameter();
    int indentSize = hPrefGrp->GetInt( "IndentSize", 4 );
    bool useSpaces = hPrefGrp->GetBool( "Spaces", false );
    QString space = useSpaces ? QString(indentSize, QLatin1Char(' '))
                       : QString::fromLatin1("\t");

    cursor.beginEditBlock();
    for (block = document()->begin(); block.isValid(); block = block.next()) {
        // get this rows indent
        int chrCount = 0, blockIndent = 0;
        bool textRow = false, mayIndent = false;
        for (int i = 0; i < block.text().size(); ++i) {
            QChar ch = block.text()[i];
            if (ch == QLatin1Char(' ')) {
                ++chrCount;
            } else if (ch == QLatin1Char('\t')) {
                chrCount += indentSize;
            } else if (ch == QLatin1Char('#')) {
                textRow = true;
                break; // comment row
            } else if (ch == QLatin1Char('\'') || ch == QLatin1Char('"')) {
                if (block.text().size() > i + 2 &&
                    block.text()[i +1] == ch && block.text()[i +2] == ch)
                {
                    if (mCommentChr == 0) {
                        mCommentChr = ch;// start a multiline comment
                    } else if (mCommentChr == ch) {
                        mCommentChr = 0;// end multiline comment
                        while (mCommentIndents > 0 && indentLevel.size()) {
                            indentLevel.removeLast();
                            --mCommentIndents;
                        }
                    }
                    textRow = true;
                    break;
                }
            } /*else if (mCommentChr != QChar(0)) {
                textRow = true;
                break; // inside multiline comment
            }*/ else if (ch.isLetterOrNumber()) {
                mayIndent = true;
                textRow = true;
                break; // text started
            }
        }
        if (!textRow) continue;

        // cancel a indent block
        while (indentLevel.size() > 0 && chrCount <= indentLevel.last()){
            // stop current indent block
            indentLevel.removeLast();
            if (mCommentChr != 0)
                --mCommentIndents;
        }

        // start a new indent block?
        QString txt = block.text();
        if (mayIndent && reNewBlock.indexIn(txt) > -1) {
            indentLevel.append(chrCount);
            blockIndent = -1; // dont indent this row
            if (mCommentChr != 0)
                ++mCommentIndents;
        }

        int pos = block.position();
        int off = block.length()-1;
        // at least one char of the block is part of the selection
        if ( pos >= selStart || pos+off >= selStart) {
            if ( pos+1 > selEnd )
                break; // end of selection reached
            int i = 0;
            for (QChar ch : block.text()) {
                if (!ch.isSpace())
                    break;
                cursor.setPosition(block.position());
                cursor.deleteChar();
                ++i;
            }
            for (i = 0; i < indentLevel.size() + blockIndent; ++i) {
                cursor.setPosition(block.position());
                cursor.insertText(space);
            }
        }
    }
    cursor.endEditBlock();
}

// ------------------------------------------------------------------------

namespace Gui {
class PythonSyntaxHighlighterP
{
public:
    PythonSyntaxHighlighterP():
        endStateOfLastPara(PythonSyntaxHighlighter::Standard)
    {
        keywords << QLatin1String("and") << QLatin1String("as")
                 << QLatin1String("assert") << QLatin1String("break")
                 << QLatin1String("class") << QLatin1String("continue")
                 << QLatin1String("def") << QLatin1String("del")
                 << QLatin1String("elif") << QLatin1String("else")
                 << QLatin1String("except") << QLatin1String("exec")
                 << QLatin1String("finally") << QLatin1String("for")
                 << QLatin1String("from") << QLatin1String("global")
                 << QLatin1String("if") << QLatin1String("import")
                 << QLatin1String("in") << QLatin1String("is")
                 << QLatin1String("lambda") << QLatin1String("None")
                 << QLatin1String("not") << QLatin1String("or")
                 << QLatin1String("pass") << QLatin1String("print")
                 << QLatin1String("raise") << QLatin1String("return")
                 << QLatin1String("try") << QLatin1String("while")
                 << QLatin1String("with") << QLatin1String("yield");
    }

    QStringList keywords;
    QString importName;
    QString importFrom;
    PythonSyntaxHighlighter::States endStateOfLastPara;
};
} // namespace Gui

/**
 * Constructs a Python syntax highlighter.
 */
PythonSyntaxHighlighter::PythonSyntaxHighlighter(QObject* parent)
    : SyntaxHighlighter(parent)
{
    d = new PythonSyntaxHighlighterP;
}

/** Destroys this object. */
PythonSyntaxHighlighter::~PythonSyntaxHighlighter()
{
    delete d;
}

/**
 * Detects all kinds of text to highlight them in the correct color.
 */
void PythonSyntaxHighlighter::highlightBlock (const QString & text)
{
  int i = 0;
  QChar prev, ch;
  int endPos = text.length() - 1;
  int blockPos = currentBlock().position();

  PythonTextBlockData *blockData = new PythonTextBlockData;
  setCurrentBlockUserData(blockData);

  d->endStateOfLastPara = static_cast<PythonSyntaxHighlighter::States>(previousBlockState());
  if (d->endStateOfLastPara < 0 || d->endStateOfLastPara > maximumUserState())
    d->endStateOfLastPara = Standard;

  while ( i < text.length() )
  {
    ch = text.at( i );

    switch ( d->endStateOfLastPara )
    {
    case Standard:
      {
        switch ( ch.unicode() )
        {
        case '#':
            // begin a comment
            setComment(i, 1);
            break;
        case '"':
          {
            // Begin either string literal or block comment
            if ((i>=2) && text.at(i-1) == QLatin1Char('"') &&
                text.at(i-2) == QLatin1Char('"'))
            {
              setDoubleQuotBlockComment(i-2, 3);
            } else {
              setDoubleQuotString(i, 1);
            }
          } break;
        case '\'':
          {
            // Begin either string literal or block comment
            if ((i>=2) && text.at(i-1) == QLatin1Char('\'') && 
                text.at(i-2) == QLatin1Char('\''))
            {
              setSingleQuotBlockComment(i-2, 3);
            } else {
              setSingleQuotString(i, 1);
            }
          } break;
        case ' ':
        case '\t':
          {
            // ignore whitespaces
          } break;
        case '(': case '[': case '{':
        case '}': case ')': case ']':
          {
            blockData->insert(ch.toLatin1(), blockPos + i);
            setOperator(i, 1);
          } break;
        case '+': case '-': case '*': case '/': 
        case ':': case '%': case '^': case '~': 
        case '!': case '=': case '<': case '>': // possibly two characters
            setOperator(i, 1);
            break;
        default:
          {
            // Check for normal text
            if ( ch.isLetter() || ch == QLatin1Char('_') )
            {
              QString buffer;
              int j=i;
              while ( ch.isLetterOrNumber() || ch == QLatin1Char('_') ) {
                buffer += ch;
                ++j;
                if (j >= text.length())
                  break; // end of text
                ch = text.at(j);
              }

              if ( d->keywords.contains( buffer ) != 0 ) {
                setKeyword(i, buffer.length());

                if ( buffer == QLatin1String("def"))
                  d->endStateOfLastPara = DefineName;
                else if ( buffer == QLatin1String("class"))
                  d->endStateOfLastPara = ClassName;
                else if ( buffer == QLatin1String("import"))
                  d->endStateOfLastPara = ImportName;
                else if ( buffer == QLatin1String("from"))
                  d->endStateOfLastPara = FromName;
              }
              else {
                setText(i, buffer.size());
              }

              // increment i
              if ( !buffer.isEmpty() )
                i = j-1;
            }
            // this is the beginning of a number
            else if ( ch.isDigit() )
            {
              setNumber(i, 1);
            }
            // probably an operator
            else if ( ch.isSymbol() || ch.isPunct() )
            {
              setOperator(i, 1);
            }
          }
        }
      } break;
    case Comment:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::Comment));
      } break;
    case Literal1:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::String));
        if ( ch == QLatin1Char('"') )
          d->endStateOfLastPara = Standard;
      } break;
    case Literal2:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::String));
        if ( ch == QLatin1Char('\'') )
          d->endStateOfLastPara = Standard;
      } break;
    case Blockcomment1:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::BlockComment));
        if ( i>=2 && ch == QLatin1Char('"') &&
            text.at(i-1) == QLatin1Char('"') &&
            text.at(i-2) == QLatin1Char('"'))
          d->endStateOfLastPara = Standard;
      } break;
    case Blockcomment2:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::BlockComment));
        if ( i>=2 && ch == QLatin1Char('\'') &&
            text.at(i-1) == QLatin1Char('\'') &&
            text.at(i-2) == QLatin1Char('\''))
          d->endStateOfLastPara = Standard;
      } break;
    case DefineName:
      {
        if ( ch.isLetterOrNumber() || ch == QLatin1Char(' ') || ch == QLatin1Char('_') )
        {
          setFormat( i, 1, this->colorByType(SyntaxHighlighter::Defname));
        }
        else
        {
          if (MatchingCharInfo::matchChar(ch.toLatin1()))
            blockData->insert(ch.toLatin1(), blockPos + i);

          if ( ch.isSymbol() || ch.isPunct() )
            setFormat(i, 1, this->colorByType(SyntaxHighlighter::Operator));
          d->endStateOfLastPara = Standard;
        }
      } break;
    case ClassName:
      {
        if ( ch.isLetterOrNumber() || ch == QLatin1Char(' ') || ch == QLatin1Char('_') )
        {
          setFormat( i, 1, this->colorByType(SyntaxHighlighter::Classname));
        }
        else
        {
          if (MatchingCharInfo::matchChar(ch.toLatin1()))
            blockData->insert(ch.toLatin1(), blockPos + i);

          if (ch.isSymbol() || ch.isPunct() )
            setFormat( i, 1, this->colorByType(SyntaxHighlighter::Operator));
          d->endStateOfLastPara = Standard;
        }
      } break;
    case Digit:
      {
        if (ch.isDigit() || ch == QLatin1Char('.'))
        {
          setFormat( i, 1, this->colorByType(SyntaxHighlighter::Number));
        }
        else
        {
          if (MatchingCharInfo::matchChar(ch.toLatin1()))
            blockData->insert(ch.toLatin1(), blockPos + i);

          if ( ch.isSymbol() || ch.isPunct() )
            setFormat( i, 1, this->colorByType(SyntaxHighlighter::Operator));
          d->endStateOfLastPara = Standard;
        }
      } break;
      case ImportName:
      {
        if ( ch.isLetterOrNumber() ||
             ch == QLatin1Char('_') || ch == QLatin1Char('*')  )
        {
          QTextCharFormat format;
          format.setForeground(this->colorByType(SyntaxHighlighter::Text));
          format.setFontWeight(QFont::Bold);
          setFormat(i, 1, format);
          d->importName += ch;
        }
        else
        {
          if (ch.isSymbol() || ch.isPunct())
          {
            setFormat(i, 1, this->colorByType(SyntaxHighlighter::Operator));
            if (ch == QLatin1Char('.'))
                d->importName += ch;
          }
          else if (prev != QLatin1Char(' '))
          {
              if (ch == QLatin1Char(' '))
              {
                //d->emitName();
              }
              else if (!d->importFrom.isEmpty()) {
                  d->importFrom.clear();
              }
          }
        }
        if (i == endPos) { // last char in row
            //d->emitName();
            d->importFrom.clear();
        }
      } break;
      case FromName:
      {
        if (prev.isLetterOrNumber() && ch == QLatin1Char(' '))
        {
            // start import statement
            d->endStateOfLastPara = Standard; //ImportName;
        }
        else if ( ch.isLetterOrNumber() || ch == QLatin1Char('_') )
        {
          QTextCharFormat format;
          format.setForeground(this->colorByType(SyntaxHighlighter::Text));
          format.setFontWeight(QFont::Bold);
          setFormat(i, 1, format);
          d->importFrom += ch;
        }
      } break;
    }

    prev = ch;
    i++;
  }

  // only block comments can have several lines
  if ( d->endStateOfLastPara != Blockcomment1 && d->endStateOfLastPara != Blockcomment2 )
  {
    d->endStateOfLastPara = Standard ;
  } 

  setCurrentBlockState(static_cast<int>(d->endStateOfLastPara));
}


void PythonSyntaxHighlighter::setComment(int pos, int len)
{
    setFormat(pos, len, this->colorByType(SyntaxHighlighter::Comment));
    d->endStateOfLastPara = Comment;
}

void PythonSyntaxHighlighter::setSingleQuotString(int pos, int len)
{
    setFormat(pos, len, this->colorByType(SyntaxHighlighter::String));
    d->endStateOfLastPara = Literal2;
}

void PythonSyntaxHighlighter::setDoubleQuotString(int pos, int len)
{
    setFormat(pos, len, this->colorByType(SyntaxHighlighter::String));
    d->endStateOfLastPara = Literal1;

}

void PythonSyntaxHighlighter::setSingleQuotBlockComment(int pos, int len)
{
    setFormat(pos, len, this->colorByType(SyntaxHighlighter::BlockComment));
    d->endStateOfLastPara = Blockcomment2;
}

void PythonSyntaxHighlighter::setDoubleQuotBlockComment(int pos, int len)
{
    setFormat(pos, len, this->colorByType(SyntaxHighlighter::BlockComment));
    d->endStateOfLastPara = Blockcomment1;
}

void PythonSyntaxHighlighter::setOperator(int pos, int len)
{
    setFormat(pos, len, this->colorByType(SyntaxHighlighter::Operator));
    d->endStateOfLastPara = Standard;
}

void PythonSyntaxHighlighter::setKeyword(int pos, int len)
{
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(this->colorByType(SyntaxHighlighter::Keyword));
    keywordFormat.setFontWeight(QFont::Bold);
    setFormat(pos, len, keywordFormat);
    d->endStateOfLastPara = Standard;
}

void PythonSyntaxHighlighter::setText(int pos, int len)
{
    setFormat(pos, len, this->colorByType(SyntaxHighlighter::Text));
    d->endStateOfLastPara = Standard;
}

void PythonSyntaxHighlighter::setNumber(int pos, int len)
{
    setFormat(pos, len, this->colorByType(SyntaxHighlighter::Number));
    d->endStateOfLastPara = Digit;
}


// --------------------------------------------------------------------------------------------


MatchingCharInfo::MatchingCharInfo():
    character(0), position(0)
{
}

MatchingCharInfo::MatchingCharInfo(const MatchingCharInfo &other)
{
    character = other.character;
    position = other.position;
}

MatchingCharInfo::MatchingCharInfo(char chr, int pos):
    character(chr), position(pos)
{
}

//static
char MatchingCharInfo::matchChar(char match)
{
    switch (match) {
    case '(':
       return ')';
    case ')':
       return '(';
    case '[':
       return ']';
    case ']':
       return '[';
    case '{':
       return '}';
    case '}':
       return '{';
    default:
       return 0;
    }
}

char MatchingCharInfo::matchingChar() const
{
    return MatchingCharInfo::matchChar(character);
}

// -------------------------------------------------------------------------------------------



PythonTextBlockData::PythonTextBlockData()
{
}

PythonTextBlockData::~PythonTextBlockData()
{
    qDeleteAll(m_matchingChrs);
}

QVector<MatchingCharInfo *> PythonTextBlockData::matchingChars()
{
    return m_matchingChrs;
}


void PythonTextBlockData::insert(MatchingCharInfo *info)
{
    int i = 0;
    while (i < m_matchingChrs.size() &&
           info->position > m_matchingChrs.at(i)->position)
        ++i;

    m_matchingChrs.insert(i, info);
}

void PythonTextBlockData::insert(char chr, int pos)
{
    MatchingCharInfo *info = new MatchingCharInfo(chr, pos);
    insert(info);
}

// -------------------------------------------------------------------------------------



PythonMatchingChars::PythonMatchingChars(TextEdit *parent):
    QObject(parent),
    m_editor(parent)
{
    // for matching chars such as (, [, { etc.
    connect(parent, SIGNAL(cursorPositionChanged()),
            this, SLOT(cursorPositionChange()));
}

PythonMatchingChars::~PythonMatchingChars()
{
}


void PythonMatchingChars::cursorPositionChange()
{
    char leftChr = 0,
         rightChr = 0;
    PythonTextBlockData *data = nullptr;

    QList<QTextEdit::ExtraSelection> selections;
    m_editor->setExtraSelections(selections);

    QTextCursor cursor = m_editor->textCursor();
    int startPos = cursor.position(),
        matchSkip = 0;

    // grab right char from cursor
    if (cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor)) {
        leftChr = cursor.selectedText()[0].toLatin1();
    }


    QTextCharFormat format;
    format.setForeground(QColor(QLatin1String("#f43218")));
    format.setBackground(QColor(QLatin1String("#f9c7f6")));

    if (leftChr == '(' || leftChr == '[' || leftChr == '{') {
        for (QTextBlock block = cursor.block();
             block.isValid(); block = block.next())
        {
            data = static_cast<PythonTextBlockData *>(block.userData());
            if (!data)
                continue;
            QVector<MatchingCharInfo *> matchingChars = data->matchingChars();
            for (const MatchingCharInfo *match : matchingChars) {
                if (match->position <= startPos)
                    continue;
                if (match->character == leftChr) {
                    ++matchSkip;
                } else if (match->matchingChar() == leftChr) {
                    if (matchSkip) {
                        --matchSkip;
                    } else {
                        m_editor->highlightText(startPos, 1, format);
                        m_editor->highlightText(match->position, 1, format);
                        return;
                    }
                }

            }
        }
    }

    // if we get here we didnt find any mathing char on right side
    // grab left char from cursor
    cursor.setPosition(startPos);
    if (cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor)) {
        rightChr = cursor.selectedText()[0].toLatin1();
    }

    if (rightChr == ')' || rightChr == ']' || rightChr == '}') {
        for (QTextBlock block = cursor.block();
             block.isValid(); block = block.previous())
        {
            data = static_cast<PythonTextBlockData *>(block.userData());
            if (!data)
                continue;
            QVector<MatchingCharInfo *> matchingChars = data->matchingChars();
            QVector<const MatchingCharInfo *>::const_iterator match = matchingChars.end();
            while (match != matchingChars.begin()) {
                --match;
                if ((*match)->position >= startPos -1)
                    continue;
                if ((*match)->character == rightChr) {
                    ++matchSkip;
                } else if ((*match)->matchingChar() == rightChr) {
                    if (matchSkip) {
                        --matchSkip;
                    } else {
                        m_editor->highlightText(startPos - 1, 1, format);
                        m_editor->highlightText((*match)->position, 1, format);
                        return;
                    }
                }

            }
        }
    }
}


#include "moc_PythonEditor.cpp"
