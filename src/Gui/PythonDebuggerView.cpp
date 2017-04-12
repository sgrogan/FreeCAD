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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QGridLayout>
# include <QApplication>
# include <QMenu>
# include <QContextMenuEvent>
# include <QTextCursor>
# include <QTextStream>
#endif


#include "PythonDebuggerView.h"
#include "PythonDebugger.h"
#include "Macro.h"
#include "Application.h"

#include <CXX/Extensions.hxx>
#include <frameobject.h>
#include <Base/Interpreter.h>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QLabel>


using namespace Gui;
using namespace Gui::DockWnd;

/* TRANSLATOR Gui::DockWnd::PythonDebuggerView */


/**
 *  Constructs a PythonDebuggerView which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
PythonDebuggerView::PythonDebuggerView(QWidget *parent)
   : QWidget(parent)
{
    setObjectName(QLatin1String("ReportOutput"));

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    m_stackLabel = new QLabel(this);
    m_stackLabel->setText(tr("Stack frames"));
    m_stackFrame = new StackFramesWidget(this);

    vLayout->addWidget(m_stackLabel);
    vLayout->addWidget(m_stackFrame);

    setLayout(vLayout);

}

PythonDebuggerView::~PythonDebuggerView()
{
}

void PythonDebuggerView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
//        tabOutput->setWindowTitle(trUtf8("Output"));
//        tabPython->setWindowTitle(trUtf8("Python console"));
//        for (int i=0; i<tabWidget->count();i++)
//            tabWidget->setTabText(i, tabWidget->widget(i)->windowTitle());
        m_stackLabel->setText(tr("Stack frames"));
    }
}


// ---------------------------------------------------------------------------

StackFramesWidget::StackFramesWidget(QWidget *parent) :
    QListWidget(parent), m_currentFrame(nullptr)
{

    PythonDebugger *debugger = Application::Instance->macroManager()->debugger();

//    connect(debugger, SIGNAL(functionCalled(PyFrameObject*)),
//            this, SLOT(functionCalled(PyFrameObject*)));
//    connect(debugger, SIGNAL(functionExited(PyFrameObject*)),
//            this, SLOT(functionExited(PyFrameObject*)));
    connect(debugger, SIGNAL(nextInstruction(PyFrameObject*)),
               this, SLOT(updateFrames(PyFrameObject*)));
}

StackFramesWidget::~StackFramesWidget()
{
}

void StackFramesWidget::functionCalled(PyFrameObject *frame)
{
    if (NULL != frame) {
        int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
        const char *filename = PyString_AsString(frame->f_code->co_filename);
        const char *funcname = PyString_AsString(frame->f_code->co_name);
        QString frameName = QString(QLatin1String("%1:%2"))
                .arg(QLatin1String(funcname))
                .arg(QString::number(line));
        QString fn = QLatin1String(filename);

        QListWidgetItem *item = new QListWidgetItem(this);
        item->setText(frameName);
        item->setToolTip(fn);
        insertItem(0, item);
        m_currentFrame = frame;
    }
}

void StackFramesWidget::functionExited(PyFrameObject *frame)
{
    QListWidgetItem *item = takeItem(0);
    if (item != nullptr)
        delete item;

    if (frame != NULL)
        m_currentFrame = frame;
}

void StackFramesWidget::updateFrames(PyFrameObject *frame)
{
    clear();

    if (frame != NULL) {
        m_currentFrame = frame;

    //PyThreadState *tstate = PyThreadState_GET();
    //if (NULL != tstate && NULL != tstate->frame) {
    //    PyFrameObject *frame = tstate->frame;
        //printf("Python stack trace:\n");
        while (NULL != frame) {
            int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
            const char *filename = PyString_AsString(frame->f_code->co_filename);
            const char *funcname = PyString_AsString(frame->f_code->co_name);
            QString frameName = QString(QLatin1String("%1:%2"))
                    .arg(QLatin1String(funcname))
                    .arg(QString::number(line));
            QString fn = QLatin1String(filename);

            QListWidgetItem *item = new QListWidgetItem(this);
            item->setText(frameName);
            item->setToolTip(fn);
            insertItem(0, item);

            frame = frame->f_back;
        }
    }
}


#include "moc_PythonDebuggerView.cpp"
