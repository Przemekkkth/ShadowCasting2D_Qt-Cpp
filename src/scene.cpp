#include "scene.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QPainter>
#include <QRgb>
#include <algorithm>

Scene::Scene(QObject *parent)
    : QGraphicsScene(parent), sprLightCast(new QImage(":/res/light_cast.png"))
{
    setSceneRect(0,0, SCREEN_SIZE.width(), SCREEN_SIZE.height());
    //
    setBackgroundBrush(QBrush(QColor(Qt::black)));
    OnUserCreate();

    connect(&m_timer, &QTimer::timeout, this, &Scene::loop);
    m_timer.start(m_loopSpeed);
    m_elapsedTimer.start();
}

bool Scene::OnUserCreate()
{
    world = new Cell[nWorldWidth * nWorldHeight];

    // Add a boundary to the world
    for (int x = 1; x < (nWorldWidth - 1); x++)
    {
        world[1 * nWorldWidth + x].exist = true;
        world[(nWorldHeight - 2) * nWorldWidth + x].exist = true;
    }

    for (int x = 1; x < (nWorldHeight - 1); x++)
    {
        world[x * nWorldWidth + 1].exist = true;
        world[x * nWorldWidth + (nWorldWidth - 2)].exist = true;
    }



    // Create some screen-sized off-screen buffers for lighting effect
    buffLightTex = new QImage(SCREEN_SIZE.width(), SCREEN_SIZE.height(), QImage::Format_RGB32);
    buffLightRay = new QImage(SCREEN_SIZE.width(), SCREEN_SIZE.height(), QImage::Format_RGB32);
    return true;
}

void Scene::OnUserUpdate()
{
    float fBlockWidth = 16.0f;
    if(m_mouseReleased)
    {
        // i = y * width + x
        int i = ((int)fSourceY / (int)fBlockWidth) * nWorldWidth + ((int)fSourceX / (int)fBlockWidth);
        world[i].exist = !world[i].exist;
        m_mouseReleased = false;
    }

    // Take a region of "TileMap" and convert it to "PolyMap" - This is done
    // every frame here, but could be a pre-processing stage depending on
    // how your final application interacts with tilemaps
    ConvertTileMapToPolyMap(0, 0, 40, 30, fBlockWidth, nWorldWidth);

    if(m_mousePressed)
    {
        CalculateVisibilityPolygon(fSourceX, fSourceY, 1000.0f);
    }

    //drawing
    clear();

    int nRaysCast = vecVisibilityPolygonPoints.size();

    auto it = std::unique(vecVisibilityPolygonPoints.begin(),
                          vecVisibilityPolygonPoints.end(),
                          [&](const PolygonPoint& t1, const PolygonPoint& t2)
    {
        return fabs(t1.x - t2.x) < 0.1f && fabs(t1.y - t2.y) < 0.1f;
    });

    vecVisibilityPolygonPoints.resize(std::distance(vecVisibilityPolygonPoints.begin(), it));
    if(m_mousePressed && vecVisibilityPolygonPoints.size() > 1)
    {
        QPainter painter1;
        painter1.begin(buffLightTex);
        //painter1.fillRect(buffLightTex->rect(), QBrush(QColor(Qt::black)));
        buffLightTex->fill(QColor(Qt::black));
        painter1.drawImage(QRectF(fSourceX - 255, fSourceY - 255, 512, 512), *sprLightCast, QRectF(0,0, 512, 512));
        painter1.end();

        QPainter painter2;
        painter2.begin(buffLightRay);
        //painter2.fillRect(buffLightTex->rect(), QBrush(QColor(Qt::black)));
        buffLightRay->fill(QColor(Qt::black));
        painter2.setBrush(QBrush(QColor(Qt::red)));
        painter2.setPen(QPen(QColor(Qt::red)));
        for (int i = 0; i < vecVisibilityPolygonPoints.size() - 1; i++)
        {
            QPainterPath path;
            path.moveTo(fSourceX, fSourceY);
            path.lineTo(fSourceX,
                        fSourceY);
            path.lineTo(vecVisibilityPolygonPoints[i].x,
                        vecVisibilityPolygonPoints[i].y);

            path.lineTo(vecVisibilityPolygonPoints[i+1].x,
                        vecVisibilityPolygonPoints[i+1].y);
            painter2.drawPath(path);
        }

        QPainterPath path;
        path.moveTo(fSourceX, fSourceY);
        path.lineTo(fSourceX,
                    fSourceY);
        path.lineTo(vecVisibilityPolygonPoints[vecVisibilityPolygonPoints.size() - 1].x,
                    vecVisibilityPolygonPoints[vecVisibilityPolygonPoints.size() - 1].y);

        path.lineTo(vecVisibilityPolygonPoints[0].x,
                    vecVisibilityPolygonPoints[0].y);
        painter2.drawPath(path);
        painter2.end();
//        QGraphicsPathItem* pathItem = new QGraphicsPathItem(path);
//        pathItem->setPen(QPen(QBrush(QColor(Qt::red)),2));
//        pathItem->setBrush(QBrush(QColor(Qt::yellow)));
//        addItem(pathItem);
        QImage resultImage(SCREEN_SIZE, QImage::Format_RGB32);
        resultImage.fill(Qt::black);
        for (int x = 0; x < SCREEN_SIZE.width(); x++)
            for (int y = 0; y < SCREEN_SIZE.height(); y++)
            {
                if(buffLightRay->pixelColor(x,y).red() > 0)
                {
                    resultImage.setPixelColor(x,y, buffLightTex->pixelColor(x, y));
                }
            }
        clear();
        QGraphicsPixmapItem* pItem = new QGraphicsPixmapItem(QPixmap::fromImage(resultImage));
        pItem->setPos(0, 0);
        addItem(pItem);

    }

    // Draw Blocks from TileMap
    for (int x = 0; x < nWorldWidth; x++)
        for (int y = 0; y < nWorldHeight; y++)
        {
            if (world[y * nWorldWidth + x].exist)
            {
                QGraphicsRectItem* rItem = new QGraphicsRectItem();
                rItem->setPos(x * fBlockWidth, y * fBlockWidth);
                rItem->setRect(0,0, fBlockWidth, fBlockWidth);
                rItem->setBrush(QBrush(QColor(Qt::blue)));
                rItem->setPen(QPen(QColor(Qt::blue)));
                addItem(rItem);
            }
        }

    // Draw Edges from PolyMap
    for (auto &e : vecEdges)
    {
        //        DrawLine(e.sx, e.sy, e.ex, e.ey);
        QGraphicsLineItem* lineItem = new QGraphicsLineItem();
        lineItem->setLine(e.sx, e.sy, e.ex, e.ey);
        QPen pen(QBrush(QColor(Qt::white)), PIXEL_SIZE.width());
        lineItem->setPen(pen);
        addItem(lineItem);

        QGraphicsEllipseItem* e1Item = new QGraphicsEllipseItem();
        e1Item->setRect(-3,-3,6,6);
        e1Item->setBrush(QBrush(QColor(Qt::red)));
        e1Item->setPos(e.sx, e.sy);
        e1Item->setZValue(1);
        addItem(e1Item);

        QGraphicsEllipseItem* e2Item = new QGraphicsEllipseItem();
        e2Item->setRect(-3,-3,6,6);
        e2Item->setBrush(QBrush(QColor(Qt::red)));
        e2Item->setPos(e.ex, e.ey);
        e2Item->setZValue(1);
        addItem(e2Item);
    }
}

void Scene::ConvertTileMapToPolyMap(int sx, int sy, int w, int h, float fBlockWidth, int pitch)
{
    // Clear "PolyMap"
    vecEdges.clear();

    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++)
            for (int j = 0; j < 4; j++)
            {
                world[(y + sy) * pitch + (x + sx)].edge_exist[j] = false;
                world[(y + sy) * pitch + (x + sx)].edge_id[j] = 0;
            }

    // Iterate through region from top left to bottom right
    for (int x = 1; x < w - 1; x++)
        for (int y = 1; y < h - 1; y++)
        {
            // Create some convenient indices
            int i = (y + sy) * pitch + (x + sx);			// This
            int n = (y + sy - 1) * pitch + (x + sx);		// Northern Neighbour
            int s = (y + sy + 1) * pitch + (x + sx);		// Southern Neighbour
            int w = (y + sy) * pitch + (x + sx - 1);	// Western Neighbour
            int e = (y + sy) * pitch + (x + sx + 1);	// Eastern Neighbour

            // If this cell exists, check if it needs edges
            if (world[i].exist)
            {
                // If this cell has no western neighbour, it needs a western edge
                if (!world[w].exist)
                {
                    // It can either extend it from its northern neighbour if they have
                    // one, or It can start a new one.
                    if (world[n].edge_exist[WEST])
                    {
                        // Northern neighbour has a western edge, so grow it downwards
                        vecEdges[world[n].edge_id[WEST]].ey += fBlockWidth;
                        world[i].edge_id[WEST] = world[n].edge_id[WEST];
                        world[i].edge_exist[WEST] = true;
                    }
                    else
                    {
                        // Northern neighbour does not have one, so create one
                        Edge edge;
                        edge.sx = (sx + x) * fBlockWidth; edge.sy = (sy + y) * fBlockWidth;
                        edge.ex = edge.sx; edge.ey = edge.sy + fBlockWidth;

                        // Add edge to Polygon Pool
                        int edge_id = vecEdges.size();
                        vecEdges.push_back(edge);

                        // Update tile information with edge information
                        world[i].edge_id[WEST] = edge_id;
                        world[i].edge_exist[WEST] = true;
                    }
                }

                // If this cell dont have an eastern neignbour, It needs a eastern edge
                if (!world[e].exist)
                {
                    // It can either extend it from its northern neighbour if they have
                    // one, or It can start a new one.
                    if (world[n].edge_exist[EAST])
                    {
                        // Northern neighbour has one, so grow it downwards
                        vecEdges[world[n].edge_id[EAST]].ey += fBlockWidth;
                        world[i].edge_id[EAST] = world[n].edge_id[EAST];
                        world[i].edge_exist[EAST] = true;
                    }
                    else
                    {
                        // Northern neighbour does not have one, so create one
                        Edge edge;
                        edge.sx = (sx + x + 1) * fBlockWidth; edge.sy = (sy + y) * fBlockWidth;
                        edge.ex = edge.sx; edge.ey = edge.sy + fBlockWidth;

                        // Add edge to Polygon Pool
                        int edge_id = vecEdges.size();
                        vecEdges.push_back(edge);

                        // Update tile information with edge information
                        world[i].edge_id[EAST] = edge_id;
                        world[i].edge_exist[EAST] = true;
                    }
                }

                // If this cell doesnt have a northern neignbour, It needs a northern edge
                if (!world[n].exist)
                {
                    // It can either extend it from its western neighbour if they have
                    // one, or It can start a new one.
                    if (world[w].edge_exist[NORTH])
                    {
                        // Western neighbour has one, so grow it eastwards
                        vecEdges[world[w].edge_id[NORTH]].ex += fBlockWidth;
                        world[i].edge_id[NORTH] = world[w].edge_id[NORTH];
                        world[i].edge_exist[NORTH] = true;
                    }
                    else
                    {
                        // Western neighbour does not have one, so create one
                        Edge edge;
                        edge.sx = (sx + x) * fBlockWidth; edge.sy = (sy + y) * fBlockWidth;
                        edge.ex = edge.sx + fBlockWidth; edge.ey = edge.sy;

                        // Add edge to Polygon Pool
                        int edge_id = vecEdges.size();
                        vecEdges.push_back(edge);

                        // Update tile information with edge information
                        world[i].edge_id[NORTH] = edge_id;
                        world[i].edge_exist[NORTH] = true;
                    }
                }

                // If this cell doesnt have a southern neignbour, It needs a southern edge
                if (!world[s].exist)
                {
                    // It can either extend it from its western neighbour if they have
                    // one, or It can start a new one.
                    if (world[w].edge_exist[SOUTH])
                    {
                        // Western neighbour has one, so grow it eastwards
                        vecEdges[world[w].edge_id[SOUTH]].ex += fBlockWidth;
                        world[i].edge_id[SOUTH] = world[w].edge_id[SOUTH];
                        world[i].edge_exist[SOUTH] = true;
                    }
                    else
                    {
                        // Western neighbour does not have one, so I need to create one
                        Edge edge;
                        edge.sx = (sx + x) * fBlockWidth; edge.sy = (sy + y + 1) * fBlockWidth;
                        edge.ex = edge.sx + fBlockWidth; edge.ey = edge.sy;

                        // Add edge to Polygon Pool
                        int edge_id = vecEdges.size();
                        vecEdges.push_back(edge);

                        // Update tile information with edge information
                        world[i].edge_id[SOUTH] = edge_id;
                        world[i].edge_exist[SOUTH] = true;
                    }
                }

            }

        }
}

void Scene::CalculateVisibilityPolygon(float ox, float oy, float radius)
{
    // Get rid of existing polygon
    vecVisibilityPolygonPoints.clear();

    // For each edge in PolyMap
    for (auto &e1 : vecEdges)
    {
        // Take the start point, then the end point (we could use a pool of
        // non-duplicated points here, it would be more optimal)
        for (int i = 0; i < 2; i++)
        {
            float rdx, rdy;
            rdx = (i == 0 ? e1.sx : e1.ex) - ox;
            rdy = (i == 0 ? e1.sy : e1.ey) - oy;

            float base_ang = atan2f(rdy, rdx);

            float ang = 0;
            // For each point, cast 3 rays, 1 directly at point
            // and 1 a little bit either side
            for (int j = 0; j < 3; j++)
            {
                if (j == 0)	ang = base_ang - 0.0001f;
                if (j == 1)	ang = base_ang;
                if (j == 2)	ang = base_ang + 0.0001f;

                // Create ray along angle for required distance
                rdx = radius * cosf(ang);
                rdy = radius * sinf(ang);

                float min_t1 = INFINITY;
                float min_px = 0, min_py = 0, min_ang = 0;
                bool bValid = false;

                // Check for ray intersection with all edges
                for (auto &e2 : vecEdges)
                {
                    // Create line segment vector
                    float sdx = e2.ex - e2.sx;
                    float sdy = e2.ey - e2.sy;

                    if (fabs(sdx - rdx) > 0.0f && fabs(sdy - rdy) > 0.0f)
                    {
                        // t2 is normalised distance from line segment start to line segment end of intersect point
                        float t2 = (rdx * (e2.sy - oy) + (rdy * (ox - e2.sx))) / (sdx * rdy - sdy * rdx);
                        // t1 is normalised distance from source along ray to ray length of intersect point
                        float t1 = (e2.sx + sdx * t2 - ox) / rdx;

                        // If intersect point exists along ray, and along line
                        // segment then intersect point is valid
                        if (t1 > 0 && t2 >= 0 && t2 <= 1.0f)
                        {
                            // Check if this intersect point is closest to source. If
                            // it is, then store this point and reject others
                            if (t1 < min_t1)
                            {
                                min_t1 = t1;
                                min_px = ox + rdx * t1;
                                min_py = oy + rdy * t1;
                                min_ang = atan2f(min_py - oy, min_px - ox);
                                bValid = true;
                            }
                        }
                    }
                }

                if(bValid)// Add intersection point to visibility polygon perimeter
                    vecVisibilityPolygonPoints.push_back({ min_ang, min_px, min_py });
            }
        }
    }

    // Sort perimeter points by angle from source. This will allow
    // us to draw a triangle fan.
    std::sort(vecVisibilityPolygonPoints.begin(), vecVisibilityPolygonPoints.end(),[&]
              (const PolygonPoint& t1, const PolygonPoint& t2){
        return t1.angle < t2.angle;
    }
    );

}

void Scene::loop()
{
    m_deltaTime = m_elapsedTimer.elapsed();
    m_elapsedTimer.restart();

    m_loopTime += m_deltaTime;
    if( m_loopTime > m_loopSpeed)
    {
        m_loopTime -= m_loopSpeed;
        OnUserUpdate();
    }
}

void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_mousePressed = false;
    if(event->button() == Qt::RightButton)
    {
        fSourceX = event->scenePos().x();
        fSourceY = event->scenePos().y();
        m_mouseReleased = true;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        fSourceX = event->scenePos().x();
        fSourceY = event->scenePos().y();
        m_mousePressed = true;
    }
    QGraphicsScene::mousePressEvent(event);
}

void Scene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    fSourceX = event->scenePos().x();
    fSourceY = event->scenePos().y();
    QGraphicsScene::mouseMoveEvent(event);
}
