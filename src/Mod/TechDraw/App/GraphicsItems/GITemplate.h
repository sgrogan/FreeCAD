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

#ifndef GITEMPLATE_HEADER
#define GITEMPLATE_HEADER

#include <QGraphicsItemGroup>

#include "../DrawTemplate.h"

namespace TechDraw {

/// Parent class representing backgrounds/templates of Drawings
class TechDrawExport GITemplate : public QGraphicsItemGroup
{
public:
    GITemplate();
    virtual ~GITemplate();

    enum {Type = QGraphicsItem::UserType + 150};
    int type() const { return Type;}

    //TODO: Check on the ownership of obj, smart pointer?
    void setTemplate(TechDraw::DrawTemplate *obj);

    TechDraw::DrawTemplate * getTemplate() { return pageTemplate; }

    virtual void clearContents() = 0;

    // TODO: Might be able to get rid of this?
    virtual void updateView(bool update = false) { draw(); }

    virtual void draw() = 0;

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    TechDraw::DrawTemplate *pageTemplate;

};  // end class GITemplate

};  // end namespace TechDraw

#endif // #ifndef GITEMPLATE_HEADER

