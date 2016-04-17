/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef DRAWINGGUI_CANVASVIEW_H
#define DRAWINGGUI_CANVASVIEW_H

#include "../App/GraphicsItems/GIPage.h"
#include "../App/GraphicsItems/GIBase.h"
#include "QGIView.h"

namespace TechDraw {
class DrawViewPart;
class DrawProjGroup;
class DrawViewDimension;
class DrawTemplate;
class DrawViewAnnotation;
class DrawViewSymbol;
class DrawViewClip;
class DrawHatch;
}

namespace TechDrawGui
{
class QGIViewDimension;
class QGITemplate;
class QGIHatch;

class TechDrawGuiExport QGVPage : public QGraphicsView, public TechDraw::GIPage
{
    Q_OBJECT

public:
    enum RendererType { Native, OpenGL, Image };

    QGVPage(TechDraw::DrawPage *page, QWidget *parent = 0);
    ~QGVPage();

    void setRenderer(RendererType type = Native);
    void drawBackground(QPainter *p, const QRectF &rect) override;

    int addView(TechDraw::GIBase *view) override;

    QGIView * addViewDimension(TechDraw::DrawViewDimension *dim);
    QGIView * addProjectionGroup(TechDraw::DrawProjGroup *view);
    QGIView * addViewPart(TechDraw::DrawViewPart *part);
    QGIView * addViewSection(TechDraw::DrawViewPart *part);
    QGIView * addDrawView(TechDraw::DrawView *view);
    QGIView * addDrawViewCollection(TechDraw::DrawViewCollection *view);
    QGIView * addDrawViewAnnotation(TechDraw::DrawViewAnnotation *view);
    QGIView * addDrawViewSymbol(TechDraw::DrawViewSymbol *view);
    QGIView * addDrawViewClip(TechDraw::DrawViewClip *view);

    void addDimToParent(QGIViewDimension* dim, QGIView* parent);
    void setPageFeature(TechDraw::DrawPage *page);
    void setPageTemplate(TechDraw::DrawTemplate *pageTemplate);

    QGITemplate * getTemplate() const;
    void removeTemplate();

    void toggleEdit(bool enable);

    /// Augments GIPage::saveSvg() so we don't save GUI-only stuff
    void saveSvg(QString filename) override;

    /// Attaches view represented by obj to this
    int attachView(App::DocumentObject *obj) override;

public Q_SLOTS:
    void setHighQualityAntialiasing(bool highQualityAntialiasing);
    void setViewBackground(bool enable);
    void setViewOutline(bool enable);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    /// As attachView (TODO: Perhaps roll this in to attachView?)
    virtual void attachTemplate(TechDraw::DrawTemplate *obj) override;

    static QColor SelectColor;
    static QColor PreselectColor;

    QGITemplate *pageTemplate;

private:
    RendererType m_renderer;

    bool drawBkg;
    QGraphicsRectItem *m_backgroundItem;
    QGraphicsRectItem *m_outlineItem;
    QBrush *bkgBrush;
    QImage m_image;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_CANVASVIEW_H
