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
    #include <QFile>
    #include <QSvgRenderer>
    #include <QGraphicsSvgItem>
#endif // #ifndef _PreComp_

#include "../DrawSVGTemplate.h"

#include "GISVGTemplate.h"

using namespace TechDraw;

GISVGTemplate::GISVGTemplate()
    : m_svgRender(new QSvgRenderer())
{
    m_svgItem = new QGraphicsSvgItem();
    m_svgItem->setSharedRenderer(m_svgRender.get());

    m_svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
    m_svgItem->setCacheMode(QGraphicsItem::NoCache);

    addToGroup(m_svgItem);
}

QVariant GISVGTemplate::itemChange( GraphicsItemChange change,
                                    const QVariant &value )
{
    return QGraphicsItemGroup::itemChange(change, value);
}

void GISVGTemplate::draw()
{
    renderSvg();
}

bool GISVGTemplate::renderSvg()
{
    auto tmplte(static_cast<TechDraw::DrawSVGTemplate *>(pageTemplate));

    if(!tmplte || !tmplte->isDerivedFrom(TechDraw::DrawSVGTemplate::getClassTypeId()))
        throw Base::Exception("Template Feature not set for QGISVGTemplate");

    auto filename(QString::fromUtf8(tmplte->PageResult.getValue()));
    if (!m_svgRender->load(filename)) {
        return false;
    }

    m_svgItem->setSharedRenderer(m_svgRender.get());

    QSize size = m_svgRender->defaultSize();
    double xaspect(tmplte->getWidth() / (double) size.width()),
           yaspect(tmplte->getHeight() / (double) size.height());

    QTransform qtrans;
    qtrans.translate(0.f, -tmplte->getHeight());
    qtrans.scale(xaspect , yaspect);
    m_svgItem->setTransform(qtrans);

    return true;
}


