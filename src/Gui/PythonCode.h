/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2017 Fredrik Johansson github.com/mumme74               *
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

#ifndef PYTHONCODE_H
#define PYTHONCODE_H


#include "Window.h"
#include "SyntaxHighlighter.h"
#include "CXX/Objects.hxx"
#include <QModelIndex>


namespace Gui {

class PythonSyntaxHighlighter;
class PythonSyntaxHighlighterP;
class PythonEditorBreakpointDlg;
class PythonCodeP;
class TextEdit;

/*
class PythonCodeObject {
public:
    PythonCodeObject(PyObject *pyObj);
    ~PythonCodeObject();
    QString value() const;
    QString typeName() const;
    QString docString() const;

    // type checks
    bool isNone() const;
    bool isType() const;
    bool isBool() const;
    bool isInt() const;
    bool isLong() const;
    bool isFloat() const;
    bool isComplex() const;
    bool isString(bool notUnicode = false) const;
    bool isUnicode() const;
    bool isTuple() const;
    bool isList() const;
    bool isDict() const;
    bool isFunction() const;
//    bool isLambda() const;
    bool isEnum() const;
    bool isCode() const;
    bool isClass() const;
    bool isInstance() const;
    bool isMethod() const;

//    bool isBuiltinFunction() const;
//    bool isBuiltInMethod() const;

    bool isModule() const;
//    bool isFile() const; // alias
    bool isSlice() const;
    bool isTraceBack() const;
    bool isFrame() const;
    bool isBuffer() const;
    bool isNotImplemented() const;
//    bool isMemberDescriptor() const;

    // end type checks


    PyObject *ptr();


    bool hasChildren() const;
    int length() const;
    QString operator[] (const QString &key) const;
    QString operator[] (int index) const;

private:
    PyObject *m_pyObject;
};
*/

/**
 * @brief Handles code inspection from the python engine internals
 */
class PythonCode : QObject
{
    Q_OBJECT
public:
    PythonCode(QObject *parent = 0);
    virtual ~PythonCode();


    // copies object and all subobjects
    PyObject* deepCopy(PyObject *obj);

    QString findFromCurrentFrame(QString varName);

    // get thee root of the parent identifier ie os.path.join
    //                                                    ^
    // must traverse from os, then os.path before os.path.join
    Py::Object getDeepObject(PyObject *obj, QString key, QString &foundKey);





private:
    PythonCodeP *d;
};

// --------------------------------------------------------------------

/**
 * Syntax highlighter for Python.
 * \author Werner Mayer
 */
class GuiExport PythonSyntaxHighlighter : public SyntaxHighlighter
{
public:
    PythonSyntaxHighlighter(QObject* parent);
    virtual ~PythonSyntaxHighlighter();

    void highlightBlock (const QString & text);

    enum States {
        Standard         = 0,     // Standard text
        Digit            = 1,     // Digits
        Comment          = 2,     // Comment begins with #
        Literal1         = 3,     // String literal beginning with "
        Literal2         = 4,     // Other string literal beginning with '
        Blockcomment1    = 5,     // Block comments beginning and ending with """
        Blockcomment2    = 6,     // Other block comments beginning and ending with '''
        ClassName        = 7,     // Text after the keyword class
        DefineName       = 8,     // Text after the keyword def
        ImportName       = 9,    // Text after import statement
        FromName         = 10,    // Text after from statement before import statement
    };

private:
    PythonSyntaxHighlighterP* d;

    inline void setComment(int pos, int len);
    inline void setSingleQuotString(int pos, int len);
    inline void setDoubleQuotString(int pos, int len);
    inline void setSingleQuotBlockComment(int pos, int len);
    inline void setDoubleQuotBlockComment(int pos, int len);
    inline void setOperator(int pos, int len);
    inline void setKeyword(int pos, int len);
    inline void setText(int pos, int len);
    inline void setNumber(int pos, int len);
    inline void setBuiltin(int pos, int len);
};

struct MatchingCharInfo
{
    char character;
    int position;
    MatchingCharInfo();
    MatchingCharInfo(const MatchingCharInfo &other);
    MatchingCharInfo(char chr, int pos);
    char matchingChar() const;
    static char matchChar(char match);
};

class PythonTextBlockData : public QTextBlockUserData
{
public:
    PythonTextBlockData();
    ~PythonTextBlockData();

    QVector<MatchingCharInfo *> matchingChars();
    void insert(MatchingCharInfo *info);
    void insert(char chr, int pos);

private:
    QVector<MatchingCharInfo *> m_matchingChrs;
};

class PythonMatchingChars : public QObject
{
    Q_OBJECT

public:
    PythonMatchingChars(TextEdit *parent);
    ~PythonMatchingChars();

private Q_SLOTS:
    void cursorPositionChange();

private:
    TextEdit *m_editor;
};

} // namespace Gui



#endif // PYTHONCODE_H
