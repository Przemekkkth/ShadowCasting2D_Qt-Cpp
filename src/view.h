#ifndef VIEW_H
#define VIEW_H

#include <QGraphicsView>
#include "scene.h"

class View : public QGraphicsView
{
    Q_OBJECT
public:
    explicit View();

signals:

private:
    Scene* m_scene;
};

#endif // VIEW_H
