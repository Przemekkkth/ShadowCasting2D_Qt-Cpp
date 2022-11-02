#include "view.h"

View::View()
    : m_scene(new Scene(this))
{
    setScene(m_scene);
    resize(m_scene->sceneRect().width(), m_scene->sceneRect().height());

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}
