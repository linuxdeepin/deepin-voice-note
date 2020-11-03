#include "leftviewlist.h"

leftviewlist::leftviewlist(QWidget *parent)
    : DTreeView(parent)
{

}

/**
 * @brief VNoteRightMenu::mouseMoveEvent 处理鼠标move事件
 * @eve 事件
 */
void leftviewlist::mouseMoveEvent(QMouseEvent *eve)
{
    if (eve->source() == Qt::MouseEventSynthesizedByQt) {
        double dist = eve->pos().y() - m_pressPointY;
        if (qAbs(dist) > 10) {
            m_mouseMoved = true;
            QDateTime current = QDateTime::currentDateTime();
            qint64 timeNow = current.toMSecsSinceEpoch();
            qint64 timerDis = timeNow - pressStartMs;
            //计算滑动速度
            double speed = ((qAbs(dist)) / timerDis) + 0.5;
            verticalScrollBar()->setSingleStep(static_cast<int>(20 * speed));
            if (dist > 0)
                verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepSub);
            else {
                verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepAdd);
            }
            pressStartMs = timeNow;
            m_pressPointY = eve->pos().y();
        }
    }
}

/**
 * @brief VNoteRightMenu::mousePressEvent 处理鼠标Press事件
 * @eve 事件
 */
void leftviewlist::mousePressEvent(QMouseEvent *eve)
{
    if (eve->source() == Qt::MouseEventSynthesizedByQt) {
        QDateTime current = QDateTime::currentDateTime();
        pressStartMs = current.toMSecsSinceEpoch();
        m_pressPointY = eve->pos().y();
    }
}

/**
 * @brief VNoteRightMenu::mouseReleaseEvent 处理鼠标Release事件
 * @eve 事件
 */
void leftviewlist::mouseReleaseEvent(QMouseEvent *eve)
{
    //正常点击状态，选择当前点击选项
    QModelIndex index = indexAt(eve->pos());
    if (selectedIndexes().count() == 0 && !m_mouseMoved && index.isValid()) {
        setCurrentIndex(index);
    }

    if (index.row() != currentIndex().row() && !m_mouseMoved && index.isValid()) {
        setCurrentIndex(index);
    }
    m_mouseMoved = false;
}
