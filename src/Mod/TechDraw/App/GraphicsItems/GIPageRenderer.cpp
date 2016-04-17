/***************************************************************************
 *   Copyright (c) 2016                    Ian Rees <ian.rees@gmail.com>   *
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
#endif // #ifndef _PreComp_

#include<QApplication>
#include<QTimer>

#include "GIPage.h"
#include "GIPageRenderer.h"

using namespace TechDraw;

GIPageRenderer::GIPageRenderer( DrawPage *page,
                                QString filename,
                                GIPageRendererFormat fmt,
                                QObject *parent) :
    QObject(parent),
    outputFilename(filename),
    outputFormat(fmt),
    sourcePage(page)
{
}

void GIPageRenderer::render()
{
    std::unique_ptr<GIPage> giPage(new GIPage(sourcePage, parent()));

    giPage->attachViews();

    switch(outputFormat) {
        case GIPageRendererFormat::SVG:
            giPage->saveSvg(outputFilename);
            break;
        default:
            assert(0);  // Unknown outputFormat
            break;
    }

    Q_EMIT finished();
}

/*static*/ void GIPageRenderer::renderToSvg(DrawPage *page, QString filename)
{
    int argc(1);
    char *argv[] = {"dummyApp"};
    {
        // Possibly useful when we have Qt5 support, from
        // http://stackoverflow.com/questions/28338470/how-to-use-qgraphicsscene-in-console-application
        // qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("minimal"));
        QApplication dummyApp(argc, argv, false);

        auto renderer(new GIPageRenderer( page, filename,
                                          GIPageRendererFormat::SVG,
                                          &dummyApp ));

        // Calls renderer->render() as soon as we enter the Qt event loop
        QTimer::singleShot(0, renderer, SLOT(render()));

        // When renderer finishes, closes out the dummy application
        connect(renderer, SIGNAL(finished()), &dummyApp, SLOT(quit()));

        // Run the event loop to handle rendering
        dummyApp.exec();
    }
}

// The GraphicsItems/ in the path is due to the CMakeLists.txt that builds
// this file living one level up.
#include "GraphicsItems/moc_GIPageRenderer.cpp"
