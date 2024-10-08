#include "gamewidget.h"
#include <QPushButton>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent), score(0), torpedoCount(10), level(1), submarine(width() / 2 - 25, height() - 50, 50, 20) {
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GameWidget::updateGame);
    timer->start(30);
    spawnShips();

    // Создаем кнопку "Играть снова", но она скрыта до конца игры
    restartButton = new QPushButton("Играть снова", this);
    restartButton->setGeometry(width() / 2 - 50, height() / 2 + 50, 100, 40);
    restartButton->hide();
    connect(restartButton, &QPushButton::clicked, this, &GameWidget::restartGame);
}

void GameWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    // Рисуем подводную лодку в виде треугольника
    painter.setBrush(Qt::gray);
    QPolygon submarineShape;
    submarineShape << QPoint(submarine.rect.left(), submarine.rect.bottom())
                   << QPoint(submarine.rect.right(), submarine.rect.bottom())
                   << QPoint(submarine.rect.center().x(), submarine.rect.top());
    painter.drawPolygon(submarineShape);

    // Рисуем корабли
    for (const Ship &ship : ships) {
        painter.setBrush(Qt::gray);
        painter.drawRect(ship.rect);
    }

    // Рисуем торпеды
    for (const Torpedo &torpedo : torpedoes) {
        painter.setBrush(Qt::red);
        painter.drawRect(torpedo.rect);
    }

    // Отображаем счет, оставшиеся торпеды и уровень
    painter.drawText(10, 10, "Score: " + QString::number(score));
    painter.drawText(10, 30, "Torpedoes left: " + QString::number(torpedoCount));
    painter.drawText(10, 50, "Level: " + QString::number(level));

    // Если игра завершена, показываем белый блок с результатами и кнопку
    if (torpedoCount == 0) {
        // Рисуем белый блок для вывода результатов
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        QRect resultRect(width() / 2 - 150, height() / 2 - 150, 300, 300);
        painter.drawRect(resultRect);

        // Отрисовываем текст результатов
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(resultRect, Qt::AlignCenter, "Game Over!\nScore: " + QString::number(score) +
                         "\nShips sunk: " + QString::number(score / 3) + "\nLevel reached: " + QString::number(level));

        // Центрируем кнопку внутри белого блока
        int buttonWidth = 120;
        int buttonHeight = 40;
        int buttonX = resultRect.center().x() - buttonWidth / 2;
        int buttonY = resultRect.bottom() - 60;  // Располагаем кнопку чуть ниже текста

        restartButton->setGeometry(buttonX, buttonY, buttonWidth, buttonHeight);
        restartButton->show();

        // Стилизуем кнопку
        restartButton->setStyleSheet(
            "QPushButton {"
            "background-color: black;"    // Зеленый фон
            "color: white;"                 // Белый текст
            "border: none;"                 // Без рамки
            "border-radius: 10px;"          // Скругленные углы
            "font-size: 13px;"              // Размер шрифта
            "padding: 10px 20px;"           // Внутренние отступы
            "}"
            "QPushButton:hover {"
            "background-color: #45a049;"    // Более темный зеленый при наведении
            "}"
        );
    }
}

void GameWidget::restartGame() {
    // Reset score, level, and torpedo count
    score = 0;
    level = 1;
    torpedoCount = 10;

    // Clear existing ships and torpedoes
    ships.clear();
    torpedoes.clear();

    // Spawn new ships for the reset game
    spawnShips();

    // Hide the restart button and start the timer again
    restartButton->hide();
    timer->start(30);

    // Update the game display
    update();
}

void GameWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && torpedoCount > 0) {
        // Запускаем торпеду из центра подводной лодки
        torpedoes.append(Torpedo(submarine.rect.center().x() - 2, submarine.rect.top() - 10, 5, 10, 5));
        torpedoCount--;
    }

    // Управляем подводной лодкой с помощью стрелок
    if (event->key() == Qt::Key_Left) {
        submarine.moveLeft();
    }
    if (event->key() == Qt::Key_Right) {
        submarine.moveRight(this->width());  // Передаем ширину окна
    }
}

void GameWidget::updateGame() {
    if (torpedoCount == 0) {
        update();  // Обновляем экран, чтобы показать результат
        return;  // Останавливаем игру, если торпед больше нет
    }

    for (Ship &ship : ships) {
        ship.move();
    }
    for (Torpedo &torpedo : torpedoes) {
        torpedo.move();
    }
    checkCollisions();
    update();
}

void GameWidget::checkCollisions() {
    for (int i = 0; i < torpedoes.size(); ++i) {
        for (int j = 0; j < ships.size(); ++j) {
            if (torpedoes[i].rect.intersects(ships[j].rect)) {
                score += ships[j].scoreValue;
                ships.removeAt(j);
                torpedoes.removeAt(i);
                spawnShips(); // Спавним новый корабль при уничтожении старого
                checkLevel(); // Проверка уровня
                return; // Выход из цикла, так как мы удалили торпеду
            }
        }
    }

    // Удаляем корабли, которые вышли за границу экрана
    for (int j = ships.size() - 1; j >= 0; --j) {
        if (ships[j].rect.right() < 0) {
            ships.removeAt(j);
            spawnShips(); // Спавним новый корабль
        }
    }
}

void GameWidget::spawnShips() {
    int randomY = qrand() % (height() - 100) + 50; // Случайное Y для нового корабля

    // Вероятность появления типа корабля в зависимости от уровня
    int randomType = qrand() % 100;
    int speed;
    int scoreValue;
    int width;
    int height;

    if (randomType < 60 - level * 10) {  // Более вероятно появление рыболовных судов на низких уровнях
        // Рыболовное судно: медленное, стоит 1 очко, большой размер
        speed = 1 + level;  // Самая медленная скорость
        scoreValue = 1;
        width = 80;  // Широкий
        height = 30;  // Высокий
    } else if (randomType < 90 - level * 5) {  // Средняя вероятность появления торгового судна
        // Торговое судно: средняя скорость, стоит 2 очка, средний размер
        speed = 2 + level;  // Средняя скорость
        scoreValue = 2;
        width = 60;  // Средний размер
        height = 25;  // Средняя высота
    } else {
        // Военное судно: быстрое, стоит 3 очка, маленький размер
        speed = 3 + level;  // Самая быстрая скорость
        scoreValue = 3;
        width = 50;  // Узкий
        height = 20;  // Низкий
    }

    // Используем 'this->width()' для получения ширины виджета
    ships.append(Ship(this->width(), randomY, width, height, speed, scoreValue));
}

void GameWidget::checkLevel() {
    int previousLevel = level;  // Запоминаем текущий уровень

    // Обновляем уровень в зависимости от количества очков
    if (score >= 40) {
        level = 5;
    }
    else if (score >= 30) {
        level = 4;
        }
    else if (score >= 20) {
        level = 3;
        }
     else if (score >= 10) {
        level = 2;
    } else if (score >= 0) {
        level = 1;
    }

    // Сброс количества торпед при повышении уровня
    if (level > previousLevel) {
        torpedoCount = 10;
    }
}
