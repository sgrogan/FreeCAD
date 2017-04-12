/***************************************************************************
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

#ifndef PYTHONDEBUGGERVIEW_H
#define PYTHONDEBUGGERVIEW_H


#include <QListWidget>
#include <Base/Console.h>
#include "DockWindow.h"
#include "Window.h"
#include <frameobject.h>

class QTabWidget;
class QLabel;

namespace Gui {
namespace DockWnd {

class StackFramesWidget;

/**
 * @brief PythonDebugger dockable widgets
 */
class PythonDebuggerView : public QWidget
{
    Q_OBJECT
public:
    PythonDebuggerView(QWidget *parent = 0);
    ~PythonDebuggerView();

protected:
    void changeEvent(QEvent *e);

private:
    StackFramesWidget *m_stackFrame;
    QLabel *m_stackLabel;
};


/**
 * @brief Displays the stackframe
 */
class StackFramesWidget : public QListWidget
{
    Q_OBJECT
public:
    StackFramesWidget(QWidget *parent = 0);
    ~StackFramesWidget();

private Q_SLOTS:
    void functionCalled(PyFrameObject *frame);
    void functionExited(PyFrameObject *frame);
    void updateFrames(PyFrameObject *frame);

private:
    PyFrameObject *m_currentFrame;
};

} // namespace DockWnd
} // namespace Gui

#endif // PYTHONDEBUGGERVIEW_H
