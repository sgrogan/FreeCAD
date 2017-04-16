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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QContextMenuEvent>
# include <QMenu>
# include <QPainter>
# include <QShortcut>
# include <QTextCursor>
#endif

#include <Python.h>
#include "PythonCode.h"
#include "PythonEditor.h"
#include "PythonDebugger.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Macro.h"
#include <Base/PyTools.h>

#include <CXX/Objects.hxx>
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>


class QModelIndex;

using namespace Gui;

PythonCodeObject::PythonCodeObject(PyObject *pyObj):
    m_pyObject(pyObj)
{
    Py_XINCREF(m_pyObject);
}

PythonCodeObject::~PythonCodeObject()
{
    Py_XDECREF(m_pyObject);
}

QString PythonCodeObject::value() const
{
    return QLatin1String(PyString_AS_STRING(m_pyObject));
}

QString PythonCodeObject::typeName() const
{
    return QLatin1String(Py_TYPE(m_pyObject)->tp_name);
}

QString PythonCodeObject::docString() const
{
    return QLatin1String(Py_TYPE(m_pyObject)->tp_doc);
}

bool PythonCodeObject::isNone() const
{
    return Py_None == m_pyObject;
}

bool PythonCodeObject::isType() const
{
    return PyType_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isBool() const
{
    return PyBool_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isInt() const
{
    return PyInt_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isLong() const
{
    return PyLong_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isFloat() const
{
    return PyLong_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isComplex() const
{
    return PyComplex_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isString(bool notUnicode) const
{
    if (PyString_Check(m_pyObject) != 0)
        return true;
    else if (notUnicode == false)
        return PyUnicode_Check(m_pyObject) != 0;
    return false;
}

bool PythonCodeObject::isUnicode() const
{
    return PyUnicode_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isDict() const
{
    return PyDict_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isFunction() const
{
    return PyFunction_Check(m_pyObject) != 0;
}

//bool PythonCodeObject::isLambda() const
//{
//    Base::PyGILStateLocker lock;
//    return PyLambda_Check(m_pyObject) != 0;
//}

bool PythonCodeObject::isEnum() const
{
    if (Py_TYPE(m_pyObject) == &PyEnum_Type)
        return true;
    return Py_TYPE(m_pyObject) == &PyReversed_Type;
}

bool PythonCodeObject::isCode() const
{
    return PyCode_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isClass() const
{
    return PyClass_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isInstance() const
{
    return PyInstance_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isMethod() const
{
    return PyMethod_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isModule() const
{
    return PyModule_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isSlice() const
{
    return PySlice_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isTraceBack() const
{
    return PyTraceBack_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isFrame() const
{
    return PyFrame_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isBuffer() const
{
    return PyBuffer_Check(m_pyObject) != 0;
}

bool PythonCodeObject::isNotImplemented() const
{
    return Py_NotImplemented == m_pyObject;
}

PyObject *PythonCodeObject::ptr()
{
    return m_pyObject;
}

//bool PythonCodeObject::isMemberDescriptor() const
//{
//    Base::PyGILStateLocker lock;
//    return PyMemberDescriptor_Check(m_pyObject) != 0;
//}

bool PythonCodeObject::isList() const
{
    return PyList_Check(m_pyObject) != 0;

}

bool PythonCodeObject::isTuple() const
{
    return PyTuple_Check(m_pyObject);
}

bool PythonCodeObject::hasChildren() const
{
    return length() > -1;
}

int PythonCodeObject::length() const
{
    Base::PyGILStateLocker lock;
    if (PyDict_Check(m_pyObject))
        return PyDict_Size(m_pyObject);
    else if (PyList_Check(m_pyObject))
        return PyList_Size(m_pyObject);
    else if (PyTuple_Check(m_pyObject))
        return PyTuple_Size(m_pyObject);
    return -1;
}

// --------------------------------------------------


namespace Gui {
struct PythonCodeP
{
    PythonCodeP()
    {
    }
    ~PythonCodeP()
    {
    }
};
} // namespace Gui


PythonCode::PythonCode(QObject *parent) :
    QObject(parent), d(new PythonCodeP)
{
}

PythonCode::~PythonCode()
{
    delete d;
}

/**
 * @brief deepcopys a object, caller takes ownership
 * @param obj to deep copy
 * @return the new obj
 */
PyObject *PythonCode::deepCopy(PyObject *obj)
{
    Base::PyGILStateLocker lock;

    // load copy module
    PyObject *deepcopyPtr = PP_Load_Attribute("copy", "deepcopy");
    if (!deepcopyPtr)
        return nullptr;

    Py_INCREF(deepcopyPtr);

    // create argument tuple
    PyObject *args = PyTuple_New(sizeof obj);
    if (!args) {
        Py_DECREF(deepcopyPtr);
        return nullptr;
    }

    Py_INCREF(args);

    if (PyTuple_SetItem(args, 0, (PyObject*)obj) != 0) {
        Py_DECREF(args);
        Py_DECREF(deepcopyPtr);
    }

    // call pythons copy.deepcopy
    PyObject *result = PyObject_CallObject(deepcopyPtr, args);
    if (!result) {
        PyErr_Clear();
    }

    return result;
}


//QString PythonCode::introspectFrame(PyObject *dictVars, const char *varName, const char *access)
//{
//    PyObject *key, *value;
//    Py_ssize_t pos = 0;
//    char *name, *valueStr;

//   // dict = traverseFromRoot(dictVars, varName, findName);

//    while (PyDict_Next(dictVars, &pos, &key, &value)) {
//        name = PyString_AS_STRING(key);
//        if (name != nullptr && strcmp(name, list[0].toLatin1()) == 0) {
//            // found correct object
//            valueStr = PyString_AS_STRING(value);
//            return QString(QLatin1String("%4 %1: %2=%3\n%5"))
//                        .arg(QLatin1String(Py_TYPE(value)->tp_name))
//                        .arg(QLatin1String(name))
//                        .arg(QLatin1String(valueStr))
//                        .arg(QLatin1String(access))
//                        .arg(QLatin1String(Py_TYPE(value)->tp_doc));
//        }
//    }

//    return QString();
//}


//PyObject* PythonCode::traverseFromRoot(PyObject *obj, char const *varName)
//{
//    // get thee root of the parent identifier ie os.path.join
//    //                                                    ^
//    // must traverse from os, then os.path before os.path.join
//    char const *c = varName;
//    int i = 1; // 1 for the \0 char
//    while (*c) {
//        i++;
//        if (*c == '.') {
//            char *var = (char*)malloc(sizeof(c) * i);
//            memcpy(var, varName, i);
//            obj = traverseFromRoot(obj, var);
//            free(var);
//            break;
//        } else if ((*c < '0') ||
//                   (*c > '9' && *c < 'A') ||
//                   (*c > '>' && *c < 'a') ||
//                   (*c > 'z'))
//        {
//            // not valid chars
//            return obj;
//        }
//    }

//    return obj;
//}


// get thee root of the parent identifier ie os.path.join
//                                                    ^
// must traverse from os, then os.path before os.path.join
QString PythonCode::findFromCurrentFrame(QString varName)
{
    Base::PyGILStateLocker locker;
    PyFrameObject *current_frame = PythonDebugger::instance()->currentFrame();
    if (current_frame == 0)
        return QString();

    QString foundKey;

    PyFrame_FastToLocals(current_frame);
    Py::Object obj = getDeepObject(current_frame->f_locals, varName, foundKey);
    if (obj.isNull())
        obj = getDeepObject(current_frame->f_globals, varName, foundKey);

    if (obj.isNull())
        obj = getDeepObject(current_frame->f_builtins, varName, foundKey);

    if (obj.isNull()) {
        return QString();
    }

    return QString(QLatin1String("%1:%3\nType:%2\n%4"))
            .arg(foundKey)
            .arg(QString::fromStdString(obj.type().as_string()))
            .arg(QString::fromStdString(obj.str().as_string()))
            .arg(QString::fromStdString(obj.repr().as_string()));

//    // found correct object
//    const char *valueStr, *reprStr, *typeStr;
//    PyObject *repr_obj;
//    repr_obj = PyObject_Repr(obj);
//    typeStr = Py_TYPE(obj)->tp_name;
//    obj = PyObject_Str(obj);
//    valueStr = PyString_AS_STRING(obj);
//    reprStr = PyString_AS_STRING(repr_obj);
//    return QString(QLatin1String("%1:%3\nType:%2\n%4"))
//                .arg(foundKey)
//                .arg(QLatin1String(typeStr))
//                .arg(QLatin1String(valueStr))
//                .arg(QLatin1String(reprStr));
}

  /**
 * @brief PythonCode::findObjFromFrame
 * get thee root of the parent identifier ie os.path.join
 *                                                     ^
 *  must traverse from os, then os.path before os.path.join
 *
 * @param obj: Search in obj as a root
 * @param key: name of var to find
 * @return Obj if found or nullptr
 */
Py::Object PythonCode::getDeepObject(PyObject *obj, QString key, QString &foundKey)
{
    Base::PyGILStateLocker locker;
    PyObject *keyObj = nullptr;
    PyObject *tmp = nullptr;

    QStringList parts = key.split(QLatin1Char('.'));
    if (!parts.size())
        return Py::Object();

    for (int i = 0; i < parts.size(); ++i) {
        keyObj = PyString_FromString(parts[i].toLatin1());
        if (keyObj != nullptr) {
            do {
                if (PyObject_HasAttr(obj, keyObj) > 0) {
                    obj = PyObject_GetAttr(obj, keyObj);
                    tmp = obj;
                    Py_XINCREF(tmp);
                } else if (PyDict_Check(obj) &&
                           PyDict_Contains(obj, keyObj))
                {
                    obj = PyDict_GetItem(obj, keyObj);
                    Py_XDECREF(tmp);
                    tmp = nullptr;
                } else
                    break; // bust out as we havent found it

                // if we get here we have found what we want
                if (i == parts.size() -1) {
                    // found the last part
                    foundKey = parts[i];
                    return Py::Object(obj);
                } else {
                    continue;
                }

            } while(false); // bust of 1 time loop
        } else {
            return Py::Object();
        }
    }
    return Py::Object();
}


// -------------------------------------------------------------------------------


namespace Gui {
class PythonSyntaxHighlighterP
{
public:
    PythonSyntaxHighlighterP():
        endStateOfLastPara(PythonSyntaxHighlighter::Standard)
    {

        Base::PyGILStateLocker lock;

        PyObject *pyObj = PyEval_GetBuiltins();
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(pyObj, &pos, &key, &value)) {
            char *name;
            name = PyString_AS_STRING(key);
            if (name != nullptr)
                builtins << QString(QLatin1String(name));
        }

        // https://docs.python.org/3/reference/lexical_analysis.html#keywords
        keywords << QLatin1String("False")    << QLatin1String("None")
                 << QLatin1String("None")     << QLatin1String("and")
                 << QLatin1String("as")       << QLatin1String("assert")
                 << QLatin1String("async")    << QLatin1String("await") //2 new kewords from 3.6
                 << QLatin1String("break")    << QLatin1String("class")
                 << QLatin1String("continue") << QLatin1String("def")
                 << QLatin1String("del")      << QLatin1String("elif")
                 << QLatin1String("else")     << QLatin1String("except")
                 << QLatin1String("finally")  << QLatin1String("for")
                 << QLatin1String("from")     << QLatin1String("global")
                 << QLatin1String("if")       << QLatin1String("import")
                 << QLatin1String("in")       << QLatin1String("is")
                 << QLatin1String("lambda")   << QLatin1String("nonlocal")
                 << QLatin1String("not")      << QLatin1String("or")
                 << QLatin1String("pass")     << QLatin1String("raise")
                 << QLatin1String("return")   << QLatin1String("try")
                 << QLatin1String("while")    << QLatin1String("with")
                 << QLatin1String("yield");

        // keywords takes precedence over builtins
        for (QString name : keywords) {
            int pos = builtins.indexOf(name);
            if (pos > -1)
                builtins.removeAt(pos);
        }

        if (!builtins.contains(QLatin1String("print")))
            keywords << QLatin1String("print"); // python 2.7 and below
    }

    QStringList keywords;
    QStringList builtins;
    QString importName;
    QString importFrom;
    PythonSyntaxHighlighter::States endStateOfLastPara;
};
} // namespace Gui

// ----------------------------------------------------------------------

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

              } else if(d->builtins.contains(buffer)) {
                setBuiltin(i, buffer.length());
              } else {
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

void PythonSyntaxHighlighter::setBuiltin(int pos, int len)
{
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(this->colorByType(SyntaxHighlighter::Builtin));
    keywordFormat.setFontWeight(QFont::Bold);
    setFormat(pos, len, keywordFormat);
    d->endStateOfLastPara = Standard;
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


#include "moc_PythonCode.cpp"
