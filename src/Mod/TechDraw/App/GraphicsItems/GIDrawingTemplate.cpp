/***************************************************************************
 *   Copyright (c) 2012-2014 Luke Parry <l.parry@warwick.ac.uk>            *
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

#include "../Geometry.h"

#include "GIDrawingTemplate.h"

using namespace TechDraw;

GIDrawingTemplate::GIDrawingTemplate()
    : pathItem(new QGraphicsPathItem)
{
    // Invert the Y for the QGraphicsPathItem with Y pointing upwards
    QTransform qtrans;
    qtrans.scale(1., -1.);

    pathItem->setTransform(qtrans);

    addToGroup(pathItem.get());
}


QVariant GIDrawingTemplate::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGraphicsItemGroup::itemChange(change, value);
}


TechDraw::DrawParametricTemplate * GIDrawingTemplate::getParametricTemplate()
{
    if(pageTemplate && pageTemplate->isDerivedFrom(TechDraw::DrawParametricTemplate::getClassTypeId()))
        return static_cast<TechDraw::DrawParametricTemplate *>(pageTemplate);
    else
        return 0;
}


void GIDrawingTemplate::draw()
{
    DrawParametricTemplate *tmplte = getParametricTemplate();
    if(!tmplte) {
        throw Base::Exception("Template Feuature not set for GIDrawingTemplate");
    }

    // Clear the previous geometry stored

    // Get a list of geometry and iterate
    const std::vector<TechDrawGeometry::BaseGeom *> &geoms =  tmplte->getGeometry();

    std::vector<TechDrawGeometry::BaseGeom *>::const_iterator it = geoms.begin();

    QPainterPath path;

    // Draw Edges
    // iterate through all the geometries
    for(; it != geoms.end(); ++it) {
        switch((*it)->geomType) {
          case TechDrawGeometry::GENERIC: {

            TechDrawGeometry::Generic *geom = static_cast<TechDrawGeometry::Generic *>(*it);

            path.moveTo(geom->points[0].fX, geom->points[0].fY);
            std::vector<Base::Vector2D>::const_iterator it = geom->points.begin();

            for(++it; it != geom->points.end(); ++it) {
                path.lineTo((*it).fX, (*it).fY);
            }
            break;
          }
          default:
            break;
        }
    }

    pathItem->setPath(path);
}

void GIDrawingTemplate::updateView(bool update)
{
    draw();
}

