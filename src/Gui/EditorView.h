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


#ifndef GUI_EDITORVIEW_H
#define GUI_EDITORVIEW_H

#include "MDIView.h"
#include "Window.h"

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QPrinter;
class QLabel;
class QPushButton;
class QLineEdit;
QT_END_NAMESPACE

namespace Gui {

class EditorViewP;
/**
 * A special view class which sends the messages from the application to
 * the editor and embeds it in a window.
 * @author Werner Mayer
 */
class GuiExport EditorView : public MDIView, public WindowParameter
{
    Q_OBJECT

public:
    enum DisplayName {
        FullName,
        FileName,
        BaseName
    };

    EditorView(QPlainTextEdit* editor, QWidget* parent);
    ~EditorView();

    QPlainTextEdit* getEditor() const;
    void setDisplayName(DisplayName);
    void OnChange(Base::Subject<const char*> &rCaller,const char* rcReason);

    const char *getName(void) const {return "EditorView";}
    void onUpdate(void){};

    bool onMsg(const char* pMsg,const char** ppReturn);
    bool onHasMsg(const char* pMsg) const;

    bool canClose(void);

    /** @name Standard actions of the editor */
    //@{
    bool open   (const QString &f);
    bool saveAs ();
    void cut    ();
    void copy   ();
    void paste  ();
    void undo   ();
    void redo   ();
    void print  ();
    void printPdf();
    void printPreview();
    void print(QPrinter*);
    void showFindBar();
    void hideFindBar();
    //@}

    QStringList undoActions() const;
    QStringList redoActions() const;
    QString fileName() const;

protected:
    void focusInEvent(QFocusEvent* e);

private Q_SLOTS:
    void checkTimestamp();
    void contentsChange(int position, int charsRemoved, int charsAdded);
    void undoAvailable(bool);
    void redoAvailable(bool);

Q_SIGNALS:
    void changeFileName(const QString&);

private:
    void setCurrentFileName(const QString &fileName);
    bool saveFile();

private:
    EditorViewP* d;
    friend class EditorSearchBar;
};

class PythonEditor;
class GuiExport PythonEditorView : public EditorView
{
    Q_OBJECT

public:
    PythonEditorView(PythonEditor* editor, QWidget* parent);
    ~PythonEditorView();

    bool onMsg(const char* pMsg,const char** ppReturn);
    bool onHasMsg(const char* pMsg) const;

public Q_SLOTS:
    void executeScript();
    void startDebug();
    void toggleBreakpoint();
    void showDebugMarker(int line);
    void hideDebugMarker();

private:
    PythonEditor* _pye;
};

/**
 * @brief The EditorSearchBar class
 * holds the widgets in the search/find in document
 */
class EditorSearchBar : public QFrame
{
    Q_OBJECT
public:
    EditorSearchBar(EditorView *parent, EditorViewP *editorViewP);
    ~EditorSearchBar();

public Q_SLOTS:
    void show();

private Q_SLOTS:
    void upSearch(bool cycle = true);
    void downSearch(bool cycle = true);
    void foundCount(int foundOcurrences);
    void searchChanged(const QString &str);
    void replace();
    void replaceAndFind();
    void replaceAll();

private:
    QLabel      *m_foundCountLabel;
    QLineEdit   *m_searchEdit;
    QPushButton *m_searchButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;
    QPushButton *m_hideButton;
    QLineEdit   *m_replaceEdit;
    QPushButton *m_replaceButton;
    QPushButton *m_replaceAndNextButton;
    QPushButton *m_replaceAllButton;

    EditorViewP *d;
};

} // namespace Gui

#endif // GUI_EDITORVIEW_H
