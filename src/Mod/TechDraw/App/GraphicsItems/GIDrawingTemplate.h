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

#ifndef GIDRAWINGTEMPLATE_HEADER
#define GIDRAWINGTEMPLATE_HEADER

#include "GITemplate.h"
#include "../DrawParametricTemplate.h"

QT_BEGIN_NAMESPACE
class QGraphicsPathItem;
QT_END_NAMESPACE

namespace TechDraw {

/// Template, but looks very similar to some of the part drawing code...
class TechDrawExport GIDrawingTemplate : virtual public GITemplate
{
public:
    GIDrawingTemplate();
    virtual ~GIDrawingTemplate() = default;
    
    enum {Type = QGraphicsItem::UserType + 151};
    int type() const { return Type; }

    void draw();
    virtual void updateView(bool update = false);

protected:
    TechDraw::DrawParametricTemplate * getParametricTemplate();

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    std::unique_ptr<QGraphicsPathItem> pathItem;
};  // end class GIDrawingTemplate

};  // end namespace TechDraw

#endif // #ifndef GIDRAWINGTEMPLATE_HEADER

