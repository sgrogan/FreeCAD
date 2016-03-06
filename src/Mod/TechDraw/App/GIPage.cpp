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
# include <QSvgGenerator>
#endif // #ifndef _PreComp_

#include "GIPage.h"

using namespace TechDraw;

GIPage::GIPage(DrawPage *page, QWidget *parent)
    : QGraphicsView(parent),
      m_page(page)
{
    assert(m_page);
    
    setScene(new QGraphicsScene(this));
}

void GIPage::saveSvg(QString filename)
{
    const QString docName( QString::fromUtf8(m_page->getDocument()->getName()) );
    const QString pageName( QString::fromUtf8(m_page->getNameInDocument()) );
    QString svgDescription = tr("Drawing page: ") + pageName +
                             tr(" exported from FreeCAD document: ") + docName;

    //Base::Console().Message("TRACE - saveSVG - page width: %d height: %d\n",width,height);    //A4 297x210
    QSvgGenerator svgGen;
    svgGen.setFileName(filename);
    svgGen.setSize(QSize((int) m_page->getPageWidth(), (int)m_page->getPageHeight()));
    svgGen.setViewBox(QRect(0, 0, m_page->getPageWidth(), m_page->getPageHeight()));
    //TODO: Exported Svg file is not quite right. <svg width="301.752mm" height="213.36mm" viewBox="0 0 297 210"... A4: 297x210
    //      Page too small (A4 vs Letter? margins?)
    //TODO: text in Qt is in mm (actually scene units).  text in SVG is points(?). fontsize in export file is too small by 1/2.835.
    //      resize all textItem before export?
    //      postprocess generated file to mult all font-size attrib by 2.835 to get pts?
    //      duplicate all textItems and only show the appropriate one for screen/print vs export?
    svgGen.setResolution(25.4000508);    // mm/inch??  docs say this is DPI
    //svgGen.setResolution(600);    // resulting page is ~12.5x9mm
    //svgGen.setResolution(96);     // page is ~78x55mm
    svgGen.setTitle(QObject::tr("FreeCAD SVG Export"));
    svgGen.setDescription(svgDescription);

    // Might need this in some cases?  Gets called at the end of toggleEdit, so
    // at least the GUI case seems to not need it.
    //scene()->update();

    QPainter p;

    p.begin(&svgGen);
    scene()->render(&p);
    p.end();
}

#include "moc_GIPage.cpp"

