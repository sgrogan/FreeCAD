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

#ifndef GIPAGERENDERER_HEADER
#define GIPAGERENDERER_HEADER

#include<QObject>

#include"../DrawPage.h"

namespace TechDraw {

/// Enumerates output formats supported by GIPageRenderer
enum class GIPageRendererFormat
{
    SVG,
};

/// Used by non-GUI FreeCAD to render Drawing pages
/*!
 * Instances of this class are constructed via the static renderTo...()
 * methods, which spin up temporary QApplication instances.  This is somewhat
 * tricky; at least with Qt 4.8, the QGraphicsScene constructor makes some
 * assumptions about there being a QApplication instance running already, and
 * will cause a segfault if that's not the case (including if a
 * QCoreApplication is running rather than a QApplication).  See
 * http://stackoverflow.com/questions/21292617/how-to-use-qgraphicsscene-with-qtgui-qguiapplication
 * or the linked Qt bug report:
 * https://bugreports.qt.io/browse/QTBUG-36413
 */
class TechDrawExport GIPageRenderer : public QObject
{
    Q_OBJECT
    private:
        /// These are instantiated only through static renderTo...() methods
        GIPageRenderer( DrawPage *page,
                        QString filename,
                        GIPageRendererFormat fmt,
                        QObject *parent = nullptr );

    public:
        static void renderToSvg(DrawPage *page, QString filename);

    public Q_SLOTS:
        void render();

    Q_SIGNALS:
        void finished();

    protected:
        QString outputFilename;
        GIPageRendererFormat outputFormat;
        DrawPage *sourcePage;
}; // end class GIPageRenderer

}; // end namespace TechDraw

#endif // #ifndef GIPAGERENDERER_HEADER

