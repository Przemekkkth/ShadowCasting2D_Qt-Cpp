#ifndef SCENE_H
#define SCENE_H

#include <QGraphicsScene>
#include <QSize>
#include <QVector>
#include <QImage>

#include "constants.h"
#include "edge.h"
#include "polygon_point.h"
#include "cell.h"

#include <QTimer>
#include <QElapsedTimer>

class Scene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Scene(QObject *parent = nullptr);

signals:

private:
    bool OnUserCreate();
    void OnUserUpdate();
    void ConvertTileMapToPolyMap(int sx, int sy, int w, int h, float fBlockWidth, int pitch);
    void CalculateVisibilityPolygon(float ox, float oy, float radius);
    QSize SCREEN_SIZE = QSize(640, 480);
    QSize PIXEL_SIZE  = QSize(2, 2);
    const int FPS = 60;

    Cell* world;
    int nWorldWidth = 40;
    int nWorldHeight = 30;

    float fSourceX;
    float fSourceY;
    bool m_mouseReleased = false;
    bool m_mousePressed = false;

    const QImage *sprLightCast;
    QImage *buffLightRay;
    QImage *buffLightTex;

    QVector<Edge> vecEdges;

    //				angle	x	y
    QVector<PolygonPoint> vecVisibilityPolygonPoints;

    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;

    float m_deltaTime = 0.0f;
    float m_loopTime  = 0.0f;
    const float m_loopSpeed = int(1000.0f/FPS);
private slots:
    void loop();
protected:
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
};

#endif // SCENE_H
