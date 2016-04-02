/***************************************************************************
 *   Copyright (c) 2012-2013 Luke Parry <l.parry@warwick.ac.uk>            *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEW_H
#define DRAWINGGUI_QGRAPHICSITEMVIEW_H


#include "../App/GIBase.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace TechDraw {
class DrawView;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGIView : public virtual TechDraw::GIBase
{
public:
    QGIView(const QPoint &position, QGraphicsScene *scene);
    QGIView();
    ~QGIView() = default;

    virtual void toggleBorder(bool state = true);
    virtual void drawBorder(void);

    void setLocked(bool state = true) { locked = true; }

    virtual void toggleCache(bool state);

    virtual void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr );
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

protected:

    bool borderVisible;
    
        // Mouse handling
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event );
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
    // Preselection events:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual QRectF customChildrenBoundingRect(void);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    QBrush m_brush;
    QPen m_decorPen;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEW_H
