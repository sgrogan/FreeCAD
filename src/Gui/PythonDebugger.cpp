/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QEventLoop>
# include <QCoreApplication>
# include <QFileInfo>
# include <QTimer>
#endif

#include "PythonDebugger.h"
#include "BitmapFactory.h"
#include <Base/Interpreter.h>
#include <Base/Console.h>

using namespace Gui;


BreakpointLine::BreakpointLine(int lineNr):
    m_lineNr(lineNr), m_hitCount(0),
    m_ignoreTo(0), m_ignoreFrom(0),
    m_disabled(false)
{
}

BreakpointLine::BreakpointLine(const BreakpointLine &other)
{
    *this = other;
}

BreakpointLine::~BreakpointLine()
{
}

BreakpointLine& BreakpointLine::operator=(const BreakpointLine& other)
{
    m_condition = other.m_condition;
    m_lineNr = other.m_lineNr;
    m_hitCount = 0; // dont copy hit counts
    m_ignoreTo = other.m_ignoreTo;
    m_ignoreFrom = other.m_ignoreFrom;
    m_disabled = other.m_disabled;
    return *this;
}

//inline
bool BreakpointLine::operator ==(int lineNr) const
{
    return m_lineNr == lineNr;
}

//inline
bool BreakpointLine::operator !=(int lineNr) const
{
    return m_lineNr != lineNr;
}

bool BreakpointLine::operator<(const BreakpointLine &other) const
{
    return m_lineNr < other.m_lineNr;
}

bool BreakpointLine::operator<(const int &line) const
{
    return m_lineNr < line;
}

int BreakpointLine::lineNr() const
{
    return m_lineNr;
}

// inline
bool BreakpointLine::hit()
{
    ++m_hitCount;
    if (m_disabled)
        return false;
    if (m_ignoreFrom > 0 && m_ignoreFrom < m_hitCount)
        return true;
    if (m_ignoreTo < m_hitCount)
        return true;

    return false;
}

void BreakpointLine::reset()
{
    m_hitCount = 0;
}

void BreakpointLine::setCondition(const QString condition)
{
    m_condition = condition;
}

const QString BreakpointLine::condition() const
{
    return m_condition;
}

void BreakpointLine::setIgnoreTo(int ignore)
{
    m_ignoreTo = ignore;
}

int BreakpointLine::ignoreTo() const
{
    return m_ignoreTo;
}

void BreakpointLine::setIgnoreFrom(int ignore)
{
    m_ignoreFrom = ignore;
}

int BreakpointLine::ignoreFrom() const
{
    return m_ignoreFrom;
}

void BreakpointLine::setDisabled(bool disable)
{
    m_disabled = disable;
}

bool BreakpointLine::disabled() const
{
    return m_disabled;
}




// -------------------------------------------------------------------------------------

Breakpoint::Breakpoint()
{
}

Breakpoint::Breakpoint(const Breakpoint& rBp)
{
    setFilename(rBp.filename());
    for (BreakpointLine bp : rBp._lines) {
        _lines.push_back(bp);
    }
}

Breakpoint& Breakpoint::operator= (const Breakpoint& rBp)
{
    if (this == &rBp)
        return *this;
    setFilename(rBp.filename());
    _lines.clear();
    for (BreakpointLine bp : rBp._lines) {
        _lines.push_back(bp);
    }
    return *this;
}

Breakpoint::~Breakpoint()
{

}

void Breakpoint::setFilename(const QString& fn)
{
    _filename = fn;
}

void Breakpoint::addLine(int line)
{
    removeLine(line);
    BreakpointLine bLine(line);
    _lines.push_back(bLine);
}

void Breakpoint::addLine(BreakpointLine bpl)
{
    removeLine(bpl.lineNr());
    _lines.push_back(bpl);
}

void Breakpoint::removeLine(int line)
{
    for (std::vector<BreakpointLine>::iterator it = _lines.begin();
         it != _lines.end(); ++it)
    {
        if (line == it->lineNr()) {
            _lines.erase(it);
            return;
        }
    }
}

bool Breakpoint::containsLine(int line)
{
    for (BreakpointLine bp : _lines) {
        if (bp == line)
            return true;
    }

    return false;
}

void Breakpoint::setDisable(int line, bool disable)
{
    for (BreakpointLine bp : _lines) {
        if (bp == line)
            bp.setDisabled(disable);
    }
}

bool Breakpoint::disabled(int line)
{
    for (BreakpointLine bp : _lines) {
        if (bp == line)
            return bp.disabled();
    }
    return false;
}

BreakpointLine *Breakpoint::getBreakPointLine(int line)
{
    for (BreakpointLine &bp : _lines) {
        if (bp == line)
            return &bp;
    }
    return nullptr;
}

// -----------------------------------------------------

void PythonDebugModule::init_module(void)
{
    PythonDebugStdout::init_type();
    PythonDebugStderr::init_type();
    PythonDebugExcept::init_type();
    static PythonDebugModule* mod = new PythonDebugModule();
    Q_UNUSED(mod);
}

PythonDebugModule::PythonDebugModule()
  : Py::ExtensionModule<PythonDebugModule>("FreeCADDbg")
{
    add_varargs_method("getFunctionCallCount", &PythonDebugModule::getFunctionCallCount,
        "Get the total number of function calls executed and the number executed since the last call to this function.");
    add_varargs_method("getExceptionCount", &PythonDebugModule::getExceptionCount,
        "Get the total number of exceptions and the number executed since the last call to this function.");
    add_varargs_method("getLineCount", &PythonDebugModule::getLineCount,
        "Get the total number of lines executed and the number executed since the last call to this function.");
    add_varargs_method("getFunctionReturnCount", &PythonDebugModule::getFunctionReturnCount,
        "Get the total number of function returns executed and the number executed since the last call to this function.");

    initialize( "The FreeCAD Python debug module" );

    Py::Dict d(moduleDictionary());
    Py::Object out(Py::asObject(new PythonDebugStdout()));
    d["StdOut"] = out;
    Py::Object err(Py::asObject(new PythonDebugStderr()));
    d["StdErr"] = err;
}

PythonDebugModule::~PythonDebugModule()
{
}

Py::Object PythonDebugModule::getFunctionCallCount(const Py::Tuple &)
{
    return Py::None();
}

Py::Object PythonDebugModule::getExceptionCount(const Py::Tuple &)
{
    return Py::None();
}

Py::Object PythonDebugModule::getLineCount(const Py::Tuple &)
{
    return Py::None();
}

Py::Object PythonDebugModule::getFunctionReturnCount(const Py::Tuple &)
{
    return Py::None();
}

// -----------------------------------------------------

void PythonDebugStdout::init_type()
{
    behaviors().name("PythonDebugStdout");
    behaviors().doc("Redirection of stdout to FreeCAD's Python debugger window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&PythonDebugStdout::write,"write to stdout");
    add_varargs_method("flush",&PythonDebugStdout::flush,"flush the output");
}

PythonDebugStdout::PythonDebugStdout()
{
}

PythonDebugStdout::~PythonDebugStdout()
{
}

Py::Object PythonDebugStdout::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonDebugStdout";
    return Py::String(s_out.str());
}

Py::Object PythonDebugStdout::write(const Py::Tuple& args)
{
    char *msg;
    //PyObject* pObj;
    ////args contains a single parameter which is the string to write.
    //if (!PyArg_ParseTuple(args.ptr(), "Os:OutputString", &pObj, &msg))
    if (!PyArg_ParseTuple(args.ptr(), "s:OutputString", &msg))
        throw Py::Exception();

    if (strlen(msg) > 0)
    {
        //send it to our stdout
        printf("%s\n",msg);

        //send it to the debugger as well
        //g_DebugSocket.SendMessage(eMSG_OUTPUT, msg);
    }
    return Py::None();
}

Py::Object PythonDebugStdout::flush(const Py::Tuple&)
{
    return Py::None();
}

// -----------------------------------------------------

void PythonDebugStderr::init_type()
{
    behaviors().name("PythonDebugStderr");
    behaviors().doc("Redirection of stderr to FreeCAD's Python debugger window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&PythonDebugStderr::write,"write to stderr");
}

PythonDebugStderr::PythonDebugStderr()
{
}

PythonDebugStderr::~PythonDebugStderr()
{
}

Py::Object PythonDebugStderr::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonDebugStderr";
    return Py::String(s_out.str());
}

Py::Object PythonDebugStderr::write(const Py::Tuple& args)
{
    char *msg;
    //PyObject* pObj;
    //args contains a single parameter which is the string to write.
    //if (!PyArg_ParseTuple(args.ptr(), "Os:OutputDebugString", &pObj, &msg))
    if (!PyArg_ParseTuple(args.ptr(), "s:OutputDebugString", &msg))
        throw Py::Exception();

    if (strlen(msg) > 0)
    {
        //send the message to our own stderr
        //dprintf(msg);

        //send it to the debugger as well
        //g_DebugSocket.SendMessage(eMSG_TRACE, msg);
        Base::Console().Error("%s", msg);
    }

    return Py::None();
}

// -----------------------------------------------------

void PythonDebugExcept::init_type()
{
    behaviors().name("PythonDebugExcept");
    behaviors().doc("Custom exception handler");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("fc_excepthook",&PythonDebugExcept::excepthook,"Custom exception handler");
}

PythonDebugExcept::PythonDebugExcept()
{
}

PythonDebugExcept::~PythonDebugExcept()
{
}

Py::Object PythonDebugExcept::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonDebugExcept";
    return Py::String(s_out.str());
}

Py::Object PythonDebugExcept::excepthook(const Py::Tuple& args)
{
    PyObject *exc, *value, *tb;
    if (!PyArg_UnpackTuple(args.ptr(), "excepthook", 3, 3, &exc, &value, &tb))
        throw Py::Exception();

    PyErr_NormalizeException(&exc, &value, &tb);

    PyErr_Display(exc, value, tb);
/*
    if (eEXCEPTMODE_IGNORE != g_eExceptionMode)
    {
        assert(tb);

        if (tb && (tb != Py_None))
        {
            //get the pointer to the frame held by the bottom traceback object - this
            //should be where the exception occurred.
            tracebackobject* pTb = (tracebackobject*)tb;
            while (pTb->tb_next != NULL) 
            {
                pTb = pTb->tb_next;
            }
            PyFrameObject* frame = (PyFrameObject*)PyObject_GetAttr((PyObject*)pTb, PyString_FromString("tb_frame"));
            EnterBreakState(frame, (PyObject*)pTb);
        }
    }*/

    return Py::None();
}

// -----------------------------------------------------

namespace Gui {
class PythonDebuggerPy : public Py::PythonExtension<PythonDebuggerPy> 
{
public:
    PythonDebuggerPy(PythonDebugger* d) : dbg(d)/*,depth(0)*/ { }
    ~PythonDebuggerPy() {}
    PythonDebugger* dbg;
    //int depth;
};

// -----------------------------------------------------

class RunningState
{
public:
    enum States {
        Stopped,
        Running,
        SingleStep,
        StepOver,
        StepOut,
        HaltOnNext
    };

    RunningState(): state(Stopped) {  }
    ~RunningState() {  }
    States operator= (States s) { state = s; return state; }
    bool operator!= (States s) { return state != s; }
    bool operator== (States s) { return state == s; }

private:
    States state;
};

// -----------------------------------------------------

struct PythonDebuggerP {
    typedef void(PythonDebuggerP::*voidFunction)(void);
    PyObject* out_o;
    PyObject* err_o;
    PyObject* exc_o;
    PyObject* out_n;
    PyObject* err_n;
    PyObject* exc_n;
    PyFrameObject* currentFrame;
    PythonDebugExcept* pypde;
    RunningState state;
    bool init, trystop, halted;
    int maxHaltLevel;
    int showStackLevel;
    QEventLoop loop;
    PyObject* pydbg;
    std::vector<Breakpoint> bps;

    PythonDebuggerP(PythonDebugger* that) :
        init(false), trystop(false), halted(false),
        maxHaltLevel(-1), showStackLevel(-1)
    {
        out_o = 0;
        err_o = 0;
        exc_o = 0;
        currentFrame = 0;
        Base::PyGILStateLocker lock;
        out_n = new PythonDebugStdout();
        err_n = new PythonDebugStderr();
        pypde = new PythonDebugExcept();
        Py::Object func = pypde->getattr("fc_excepthook");
        exc_n = Py::new_reference_to(func);
        pydbg = new PythonDebuggerPy(that);
    }
    ~PythonDebuggerP()
    {
        Base::PyGILStateLocker lock;
        Py_DECREF(out_n);
        Py_DECREF(err_n);
        Py_DECREF(exc_n);
        Py_DECREF(pypde);
        Py_DECREF(pydbg);
    }
};
}

// ---------------------------------------------------------------

PythonDebugger::PythonDebugger()
  : d(new PythonDebuggerP(this))
{
    globalInstance = this;

    typedef void (*STATICFUNC)( );
    STATICFUNC fp = PythonDebugger::finalizeFunction;
    Py_AtExit(fp);
}

PythonDebugger::~PythonDebugger()
{
    stop();
    delete d;
    globalInstance = nullptr;
}

Breakpoint PythonDebugger::getBreakpoint(const QString& fn) const
{
    for (std::vector<Breakpoint>::const_iterator it = d->bps.begin(); it != d->bps.end(); ++it) {
        if (fn == it->filename()) {
            return Breakpoint(*it);
        }
    }

    return Breakpoint();
}

BreakpointLine *PythonDebugger::getBreakpointLine(const QString fn, int line)
{
    for (Breakpoint &bp : d->bps) {
        if (fn == bp.filename())
            return bp.getBreakPointLine(line);
    }
    return nullptr;
}

void PythonDebugger::setBreakpoint(const QString fn, int line)
{
    for (Breakpoint &bp : d->bps) {
        if (fn == bp.filename()) {
            bp.removeLine(line);
            bp.addLine(line);
            return;
        }
    }

    Breakpoint bp;
    bp.setFilename(fn);
    bp.addLine(line);
    d->bps.push_back(bp);
}

void PythonDebugger::setBreakpoint(const QString fn, BreakpointLine bpl)
{
    for (Breakpoint &bp : d->bps) {
        if (fn == bp.filename()) {
            bp.removeLine(bpl.lineNr());
            bp.addLine(bpl.lineNr());
            return;
        }
    }

    Breakpoint bp;
    bp.setFilename(fn);
    bp.addLine(bpl);
    d->bps.push_back(bp);
}

void PythonDebugger::deleteBreakpoint(const QString fn, int line)
{
    for (Breakpoint &bp : d->bps) {
        if (fn == bp.filename()) {
            bp.removeLine(line);
        }
    }
}

void PythonDebugger::setDisableBreakpoint(const QString fn, int line, bool disable)
{
    for (Breakpoint &bp : d->bps) {
        if (fn == bp.filename()) {
            bp.setDisable(line, disable);
        }
    }
}



bool PythonDebugger::toggleBreakpoint(int line, const QString& fn)
{
    for (Breakpoint &bp : d->bps) {
        if (fn == bp.filename()) {
            if (bp.containsLine(line)) {
                bp.removeLine(line);
                return false;
            }
            bp.addLine(line);
            return true;
        }
    }

    Breakpoint bp;
    bp.setFilename(fn);
    bp.addLine(line);
    d->bps.push_back(bp);
    return true;
}

void PythonDebugger::runFile(const QString& fn)
{
    try {
        if (d->state != RunningState::HaltOnNext)
            d->state = RunningState::Running;
        QByteArray pxFileName = fn.toUtf8();
#ifdef FC_OS_WIN32
        Base::FileInfo fi((const char*)pxFileName);
        FILE *fp = _wfopen(fi.toStdWString().c_str(),L"r");
#else
        FILE *fp = fopen((const char*)pxFileName,"r");
#endif
        if (!fp) {
            d->state = RunningState::Stopped;
            return;
        }

        Base::PyGILStateLocker locker;
        PyObject *module, *dict;
        module = PyImport_AddModule("__main__");
        dict = PyModule_GetDict(module);
        dict = PyDict_Copy(dict);
        if (PyDict_GetItemString(dict, "__file__") == NULL) {
#if PY_MAJOR_VERSION >= 3
            PyObject *f = PyUnicode_FromString((const char*)pxFileName);
#else
            PyObject *f = PyString_FromString((const char*)pxFileName);
#endif
            if (f == NULL) {
                fclose(fp);
                d->state = RunningState::Stopped;
                return;
            }
            if (PyDict_SetItemString(dict, "__file__", f) < 0) {
                Py_DECREF(f);
                fclose(fp);
                d->state = RunningState::Stopped;
                return;
            }
            Py_DECREF(f);
        }

        PyObject *result = PyRun_File(fp, (const char*)pxFileName, Py_file_input, dict, dict);
        fclose(fp);
        Py_DECREF(dict);

        if (!result) {
            PyErr_Print();
            d->state = RunningState::Stopped;
         } else
            Py_DECREF(result);
    }
    catch (const Base::PyException&/* e*/) {
        //PySys_WriteStderr("Exception: %s\n", e.what());
        PyErr_Clear();
    }
    catch (...) {
        Base::Console().Warning("Unknown exception thrown during macro debugging\n");
        //PyErr_Clear();
    }

    d->state = RunningState::Stopped;
}

bool PythonDebugger::isRunning() const
{
    return d->state != RunningState::Stopped;
}

bool PythonDebugger::isHalted() const
{
    Base::PyGILStateLocker locker;
    return d->halted;
}

bool PythonDebugger::start()
{
    if (d->state == RunningState::Stopped)
        d->state = RunningState::Running;

    if (d->init)
        return false;

    d->init = true;
    d->trystop = false;

    { // thread lock code block
        Base::PyGILStateLocker lock;
        d->out_o = PySys_GetObject("stdout");
        d->err_o = PySys_GetObject("stderr");
        d->exc_o = PySys_GetObject("excepthook");

        PySys_SetObject("stdout", d->out_n);
        PySys_SetObject("stderr", d->err_n);
        PySys_SetObject("excepthook", d->exc_o);

        PyEval_SetTrace(tracer_callback, d->pydbg);
    } // end threadlock codeblock

    Q_EMIT started();
    return true;
}

bool PythonDebugger::stop()
{
    if (!d->init)
        return false;
    if (d->halted)
        _signalNextStep();

    { // threadlock code block
        Base::PyGILStateLocker lock;
        PyErr_Clear();
        PyEval_SetTrace(NULL, NULL);
        PySys_SetObject("stdout", d->out_o);
        PySys_SetObject("stderr", d->err_o);
        PySys_SetObject("excepthook", d->exc_o);
        d->init = false;
    } // end thread lock code block
    d->currentFrame = nullptr;
    d->state = RunningState::Stopped;
    d->halted = false;
    d->trystop = false;
    Q_EMIT stopped();
    return true;
}

void PythonDebugger::tryStop()
{
    d->trystop = true;
    _signalNextStep();
}

void PythonDebugger::haltOnNext()
{
    start();
    d->state = RunningState::HaltOnNext;
}

void PythonDebugger::stepOver()
{
    d->state = RunningState::StepOver;
    d->maxHaltLevel = callDepth();
    _signalNextStep();
}

void PythonDebugger::stepInto()
{
    d->state = RunningState::SingleStep;
    _signalNextStep();
}

void PythonDebugger::stepOut()
{
    d->state = RunningState::StepOut;
    d->maxHaltLevel = callDepth() -1;
    if (d->maxHaltLevel < 0)
        d->maxHaltLevel = 0;
    _signalNextStep();
}

void PythonDebugger::stepContinue()
{
    d->state = RunningState::Running;
    _signalNextStep();
}

//void PythonDebugger::showDebugMarker(const QString& fn, int line)
//{
//    PythonEditorView* edit = 0;
//    QList<QWidget*> mdis = getMainWindow()->windows();
//    for (QList<QWidget*>::iterator it = mdis.begin(); it != mdis.end(); ++it) {
//        edit = qobject_cast<PythonEditorView*>(*it);
//        if (edit && edit->fileName() == fn)
//            break;
//    }

//    if (!edit) {
//        PythonEditor* editor = new PythonEditor();
//        editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
//        edit = new PythonEditorView(editor, getMainWindow());
//        edit->open(fn);
//        edit->resize(400, 300);
//        getMainWindow()->addWindow(edit);
//    }

//    getMainWindow()->setActiveWindow(edit);
//    edit->showDebugMarker(line);
//}

//void PythonDebugger::hideDebugMarker(const QString& fn)
//{
//    PythonEditorView* edit = 0;
//    QList<QWidget*> mdis = getMainWindow()->windows();
//    for (QList<QWidget*>::iterator it = mdis.begin(); it != mdis.end(); ++it) {
//        edit = qobject_cast<PythonEditorView*>(*it);
//        if (edit && edit->fileName() == fn) {
//            edit->hideDebugMarker();
//            break;
//        }
//    }
//}

PyFrameObject *PythonDebugger::currentFrame() const
{
    Base::PyGILStateLocker locker;
    if (d->showStackLevel < 0)
        return d->currentFrame;

    // lets us show different stacks
    PyFrameObject *fr = d->currentFrame;
    int i = callDepth() - d->showStackLevel;
    while (nullptr != fr && i > 1) {
        fr = fr->f_back;
        --i;
    }

    return fr;

}

int PythonDebugger::callDepth(const PyFrameObject *frame) const
{
    PyFrameObject const *fr = frame;
    int i = 0;
    Base::PyGILStateLocker locker;
    while (nullptr != fr) {
        fr = fr->f_back;
        ++i;
    }

    return i;
}

int PythonDebugger::callDepth() const
{
    Base::PyGILStateLocker locker;
    PyFrameObject const *fr = d->currentFrame;
    return callDepth(fr);
}

bool PythonDebugger::setStackLevel(int level)
{
    if (!d->halted)
        return false;

    --level; // 0 based

    int calls = callDepth() - 1;
    if (calls >= level && level >= 0) {
        if (d->showStackLevel == level)
            return true;

        Base::PyGILStateLocker lock;

        if (calls == level)
            d->showStackLevel = -1;
        else
            d->showStackLevel = level;

        // notify observers
        PyFrameObject *frame = currentFrame();
        if (frame) {
            int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
            QString file = QString::fromUtf8(PyString_AsString(frame->f_code->co_filename));
            Q_EMIT haltAt(file, line);
            Q_EMIT nextInstruction(frame);
        }
    }

    return false;
}

// is owned by macro manager which handles delete
PythonDebugger* PythonDebugger::globalInstance = nullptr;

PythonDebugger *PythonDebugger::instance()
{
    if (globalInstance == nullptr)
        // is owned by macro manager which handles delete
        globalInstance = new PythonDebugger;
    return globalInstance;
}

// http://www.koders.com/cpp/fidBA6CD8A0FE5F41F1464D74733D9A711DA257D20B.aspx?s=PyEval_SetTrace
// http://code.google.com/p/idapython/source/browse/trunk/python.cpp
// http://www.koders.com/cpp/fid191F7B13CF73133935A7A2E18B7BF43ACC6D1784.aspx?s=PyEval_SetTrace
// http://stuff.mit.edu/afs/sipb/project/python/src/python2.2-2.2.2/Modules/_hotshot.c
// static
int PythonDebugger::tracer_callback(PyObject *obj, PyFrameObject *frame, int what, PyObject * /*arg*/)
{
    PythonDebuggerPy* self = static_cast<PythonDebuggerPy*>(obj);
    PythonDebugger* dbg = self->dbg;
    if (dbg->d->trystop) {
        PyErr_SetInterrupt();
        dbg->d->state = RunningState::Stopped;
        return 0;
    }
    QCoreApplication::processEvents();
    //int no;

    //no = frame->f_tstate->recursion_depth;
    //std::string funcname = PyString_AsString(frame->f_code->co_name);
#if PY_MAJOR_VERSION >= 3
    QString file = QString::fromUtf8(PyUnicode_AsUTF8(frame->f_code->co_filename));
#else
    QString file = QString::fromUtf8(PyString_AsString(frame->f_code->co_filename));
#endif
    switch (what) {
    case PyTrace_CALL:
        if (dbg->d->state != RunningState::Running) {
            try {
                Q_EMIT dbg->functionCalled(frame);
            } catch(...){ } // might throw
        }
        return 0;
    case PyTrace_RETURN:
        if (dbg->d->state != RunningState::Running) {
            try {
                Q_EMIT dbg->functionExited(frame);
            } catch (...) { }
        }
        return 0;
    case PyTrace_LINE:
        {

            int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
            bool halt = false;
            if (dbg->d->state == RunningState::SingleStep ||
                dbg->d->state == RunningState::HaltOnNext)
            {
                halt = true;
            } else if((dbg->d->state == RunningState::StepOver ||
                       dbg->d->state == RunningState::StepOut) &&
                      dbg->d->maxHaltLevel >= dbg->callDepth(frame))
            {
                halt = true;
            } else { // RunningState
                BreakpointLine *bp = dbg->getBreakpointLine(file, line);
                if (bp != nullptr) {
                    if (bp->condition().size()) {
                        halt = PythonDebugger::evalCondition(bp->condition().toLatin1(),
                                                             frame);
                    } else if (bp->hit()) {
                        halt = true;
                    }
                }
            }

            if (halt) {

                while(dbg->d->halted) {
                    // already halted, must be another thread here
                    // halt until current thread releases
                    QCoreApplication::processEvents();
                }

                QEventLoop loop;
                {   // threadlock block
                    Base::PyGILStateLocker locker;
                    dbg->d->currentFrame = frame;

                    if (!dbg->d->halted) {
                        try {
                            Q_EMIT dbg->functionCalled(frame);
                        } catch (...) { }
                    }
                    dbg->d->halted = true;
                    try {
                        Q_EMIT dbg->haltAt(file, line);
                        Q_EMIT dbg->nextInstruction(frame);
                    } catch(...) { }
                }   // end threadlock block
                QObject::connect(dbg, SIGNAL(_signalNextStep()), &loop, SLOT(quit()));
                loop.exec();
                {   // threadlock block
                    Base::PyGILStateLocker locker;
                    dbg->d->halted = false;
                }   // end threadlock block
                try {
                    Q_EMIT dbg->releaseAt(file, line);
                } catch (...) { }
            }

            return 0;
        }
    case PyTrace_EXCEPTION:
        return 0;
    case PyTrace_C_CALL:
        return 0;
    case PyTrace_C_EXCEPTION:
        return 0;
    case PyTrace_C_RETURN:
        return 0;
    default:
        /* ignore PyTrace_EXCEPTION */
        break;
    }
    return 0;
}

// credit to: https://github.com/jondy/pyddd/blob/master/ipa.c
// static
bool PythonDebugger::evalCondition(const char *condition, PyFrameObject *frame)
{
    /* Eval breakpoint condition */
    PyObject *result;

    /* Use empty filename to avoid obj added to object entry table */
    PyObject* exprobj = Py_CompileStringFlags(condition, "", Py_eval_input, NULL);
    if (!exprobj) {
        PyErr_Clear();
        return false;
    }

    /* Clear flag use_tracing in current PyThreadState to avoid
         tracing evaluation self
      */
    frame->f_tstate->use_tracing = 0;
    result = PyEval_EvalCode((PyCodeObject*)exprobj,
                             frame->f_globals,
                             frame->f_locals);
    frame->f_tstate->use_tracing = 1;
    Py_DecRef(exprobj);

    if (result == NULL) {
        PyErr_Clear();
        return false;
    }

    if (PyObject_IsTrue(result) != 1) {
        Py_DecRef(result);
        return false;
    }
    Py_DecRef(result);

    return true;
}

// static
void PythonDebugger::finalizeFunction()
{
    if (globalInstance != nullptr)
        globalInstance->_signalNextStep(); // release a pending halt on app close
}

#include "moc_PythonDebugger.cpp"
