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

#ifndef GIVIEWBASE_HEADER
#define GIVIEWBASE_HEADER

#include <QFont>
#include <QGraphicsItemGroup>
#include <QObject>
#include <QPen>

#include "DrawView.h"

namespace TechDraw {

class TechDrawExport GIBase : public QGraphicsItemGroup
{
public:
    GIBase(const QPoint &pos, QGraphicsScene *scene);
    virtual ~GIBase() {};

    enum {Type = QGraphicsItem::UserType + 101};
    int type() const { return Type;}

    const char * getViewName() const;

    void setViewFeature(TechDraw::DrawView *obj);
    TechDraw::DrawView * getViewObject() const;    

    /// Methods to ensure that Y-Coordinates are orientated correctly.
    /// @{
    void setPosition(qreal x, qreal y);
    inline qreal getY() { return y() * -1; }
    bool isInnerView() { return m_innerView; }
    void isInnerView(bool state) { m_innerView = state; }
    double getYInClip(double y);
    /// @}

    /// Used for constraining views to line up eg in a Projection Group
    void alignTo(QGraphicsItem*, const QString &alignment);

    virtual void updateView(bool update = false);
    virtual void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr );

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    DrawView *viewObj;

    bool locked;
    bool m_innerView;  //View is inside another View

    std::string viewName;

    QColor m_colCurrent;
    QColor m_colNormal;
    QColor m_colPre;
    QColor m_colSel;
    QFont m_font;
    QGraphicsRectItem* m_border;
    QGraphicsTextItem* m_label;
    QHash<QString, QGraphicsItem*> alignHash;
    QPen m_pen;
};

} // end namespace TechDraw

#endif // #ifndef GIVIEWBASE_HEADER

