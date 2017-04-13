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


#ifndef GUI_PYTHONDEBUG_H
#define GUI_PYTHONDEBUG_H

#include <CXX/Extensions.hxx>
#include <frameobject.h>
#include <string>
#include <vector>

namespace Gui {

class BreakpointLine {
public:
    //BreakPointLine(); // we should not create without lineNr
    BreakpointLine(int lineNr);
    BreakpointLine(const BreakpointLine &other);
    ~BreakpointLine();

    BreakpointLine& operator=(const BreakpointLine &other);
    bool operator==(int lineNr) const;
    bool operator!=(int lineNr) const;
    inline bool operator<(const BreakpointLine &other) const;
    bool operator<(const int &line) const;

    int lineNr() const;

    inline bool hit();
    void reset();
    int hitCount() const { return m_hitCount; }

    /**
     * @brief setCondition a python expression (True triggers breakpoint)
     * @param condition a python expression
     */
    void setCondition(const QString condition);
    const QString condition() const;
    /**
     * @brief setIgnoreTo ignores hits on this line up to ignore hits
     * @param ignore
     */
    void setIgnoreTo(int ignore);
    int ignoreTo() const;
    /**
     * @brief setIgnoreFrom ignores hits on this line from ignore hits
     * @param ignore
     */
    void setIgnoreFrom(int ignore);
    int ignoreFrom() const;

    void setDisabled(bool disable);
    bool disabled() const;

private:
    QString m_condition;
    int m_lineNr;
    int m_hitCount;
    int m_ignoreTo;
    int m_ignoreFrom;
    bool m_disabled;
};

class Breakpoint
{
public:
    Breakpoint();
    Breakpoint(const Breakpoint&);
    Breakpoint& operator=(const Breakpoint&);

    ~Breakpoint();

    const QString& filename() const;
    void setFilename(const QString& fn);

    bool operator ==(const Breakpoint& bp);
    bool operator ==(const QString& fn);

    void addLine(int line);
    void addLine(BreakpointLine bpl);
    void removeLine(int line);
    bool containsLine(int line);
    void setDisable(int line, bool disable);
    bool disabled(int line);

    int countLines()const;

    bool checkBreakpoint(const QString& fn, int line);
    BreakpointLine *getBreakPointLine(int line);

private:
    QString _filename;
    std::vector<BreakpointLine> _lines;
};

inline const QString& Breakpoint::filename()const
{
    return _filename;
}

inline int Breakpoint::countLines()const
{
    return static_cast<int>(_lines.size());
}

inline bool Breakpoint::checkBreakpoint(const QString& fn, int line)
{
    assert(!_filename.isEmpty());
    for (BreakpointLine &bp : _lines) {
        if (bp == line)
            return fn == _filename;
    }

    return false;
}

inline bool Breakpoint::operator ==(const Breakpoint& bp)
{
    return _filename == bp._filename;
}

inline bool Breakpoint::operator ==(const QString& fn)
{
    return _filename == fn;
}

/**
 * @author Werner Mayer
 */
class GuiExport PythonDebugModule : public Py::ExtensionModule<PythonDebugModule>
{
public:
    static void init_module(void);

    PythonDebugModule();
    virtual ~PythonDebugModule();

private:
    Py::Object getFunctionCallCount(const Py::Tuple &a);
    Py::Object getExceptionCount(const Py::Tuple &a);
    Py::Object getLineCount(const Py::Tuple &a);
    Py::Object getFunctionReturnCount(const Py::Tuple &a);
};

/**
 * @author Werner Mayer
 */
class GuiExport PythonDebugStdout : public Py::PythonExtension<PythonDebugStdout> 
{
public:
    static void init_type(void);    // announce properties and methods

    PythonDebugStdout();
    ~PythonDebugStdout();

    Py::Object repr();
    Py::Object write(const Py::Tuple&);
    Py::Object flush(const Py::Tuple&);
};

/**
 * @author Werner Mayer
 */
class GuiExport PythonDebugStderr : public Py::PythonExtension<PythonDebugStderr> 
{
public:
    static void init_type(void);    // announce properties and methods

    PythonDebugStderr();
    ~PythonDebugStderr();

    Py::Object repr();
    Py::Object write(const Py::Tuple&);
};

/**
 * @author Werner Mayer
 */
class GuiExport PythonDebugExcept : public Py::PythonExtension<PythonDebugExcept> 
{
public:
    static void init_type(void);    // announce properties and methods

    PythonDebugExcept();
    ~PythonDebugExcept();

    Py::Object repr();
    Py::Object excepthook(const Py::Tuple&);
};

class GuiExport PythonDebugger : public QObject
{
    Q_OBJECT

public:
    PythonDebugger();
    ~PythonDebugger();
    Breakpoint getBreakpoint(const QString&) const;
    BreakpointLine *getBreakpointLine(const QString fn, int line);
    void setBreakpoint(const QString fn, int line);
    void setBreakpoint(const QString fn, BreakpointLine bpl);
    void deleteBreakpoint(const QString fn, int line);
    void setDisableBreakpoint(const QString fn, int line, bool disable);
    bool toggleBreakpoint(int line, const QString&);
    void runFile(const QString& fn);
    bool isRunning() const;
    bool isHalted() const;
    PyFrameObject *currentFrame() const;
    /**
     * @brief callDepth: gets the call depth of frame
     * @param frame: if null use currentFrame
     * @return the call depth
     */
    int callDepth(const PyFrameObject *frame) const;
    int callDepth() const;
    static PythonDebugger *instance();

public Q_SLOTS:
    bool start();
    bool stop();
    void tryStop();
    void haltOnNext();
    void stepOver();
    void stepInto();
    void stepOut();
    void stepContinue();

Q_SIGNALS:
    void _signalNextStep(); // used internally
    void started();
    void stopped();
    void nextInstruction(PyFrameObject *frame);
    void functionCalled(PyFrameObject *frame);
    void functionExited(PyFrameObject *frame);
    void haltAt(const QString &filename, int line);
    void releaseAt(const QString &filename, int line);

private:
    static int tracer_callback(PyObject *obj, PyFrameObject *frame, int what, PyObject *arg);
    static bool evalCondition(const char *condition, PyFrameObject *frame);
    static void finalizeFunction(); // called when interpreter finalizes

    struct PythonDebuggerP* d;
    static PythonDebugger *globalInstance;
};

} // namespace Gui

#endif // GUI_PYTHONDEBUG_H
