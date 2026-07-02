#include "view/GanttExporter.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <map>

namespace view {
namespace {

struct PlotArea {
    sf::IntRect rect;
    float xmin;
    float xmax;
    float ymin;
    float ymax;
};

const sf::Color BG_COLOR(28, 30, 38);
const sf::Color GRID_COLOR(55, 60, 75);
const sf::Color AXIS_COLOR(200, 200, 210);
const sf::Color READY_BORDER(120, 130, 150);
const sf::Color SUSPENDED_COLOR(15, 15, 18);
const sf::Color SUSPENDED_IO_STRIPE   ( 60, 130, 220);
const sf::Color SUSPENDED_MUTEX_STRIPE(220,  80,  80);
const sf::Color MUTEX_LOCK_COLOR      (245, 200,  60);
const sf::Color MUTEX_UNLOCK_COLOR    ( 90, 200, 120);
const sf::Color IO_ICON_COLOR         ( 60, 130, 220);
const sf::Color LOTTERY_COLOR(255, 215, 0);
const sf::Color ARRIVAL_COLOR(40, 220, 80);
const sf::Color TERMINATION_COLOR(230, 60, 60);

sf::Vector2i mapToPixel(float x, float y, const PlotArea& p) {
    float nx = (x - p.xmin) / (p.xmax - p.xmin);
    float ny = (y - p.ymin) / (p.ymax - p.ymin);
    return {
        p.rect.left + static_cast<int>(std::round(nx * p.rect.width)),
        p.rect.top + static_cast<int>(std::round((1.0f - ny) * p.rect.height))
    };
}

void putPixel(sf::Image& image, int x, int y, sf::Color color) {
    auto size = image.getSize();
    if (x < 0 || y < 0 || x >= static_cast<int>(size.x) || y >= static_cast<int>(size.y)) return;
    image.setPixel(static_cast<unsigned>(x), static_cast<unsigned>(y), color);
}

void fillRect(sf::Image& image, int left, int top, int width, int height, sf::Color color) {
    if (width <= 0 || height <= 0) return;
    for (int y = top; y < top + height; ++y) {
        for (int x = left; x < left + width; ++x) {
            putPixel(image, x, y, color);
        }
    }
}

void outlineRect(sf::Image& image, int left, int top, int width, int height, sf::Color color) {
    if (width <= 0 || height <= 0) return;
    for (int x = left; x < left + width; ++x) {
        putPixel(image, x, top, color);
        putPixel(image, x, top + height - 1, color);
    }
    for (int y = top; y < top + height; ++y) {
        putPixel(image, left, y, color);
        putPixel(image, left + width - 1, y, color);
    }
}

void drawLine(sf::Image& image, int x1, int y1, int x2, int y2, sf::Color color) {
    int dx = std::abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;
    int dy = -std::abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        putPixel(image, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void drawGrid(sf::Image& image, const PlotArea& p, int xTicks, int yTicks) {
    if (xTicks <= 0) xTicks = 1;
    if (yTicks <= 0) yTicks = 1;

    for (int i = 0; i <= xTicks; ++i) {
        int x = p.rect.left + (p.rect.width * i) / xTicks;
        drawLine(image, x, p.rect.top, x, p.rect.top + p.rect.height, GRID_COLOR);
    }
    for (int j = 0; j <= yTicks; ++j) {
        int y = p.rect.top + (p.rect.height * j) / yTicks;
        drawLine(image, p.rect.left, y, p.rect.left + p.rect.width, y, GRID_COLOR);
    }
    outlineRect(image, p.rect.left, p.rect.top, p.rect.width, p.rect.height, AXIS_COLOR);
}

void drawRunBlock(sf::Image& image, const PlotArea& p, int tick, int row,
                  sf::Color color, bool lottery) {
    sf::Vector2i bl = mapToPixel(static_cast<float>(tick), row - 0.4f, p);
    sf::Vector2i tr = mapToPixel(static_cast<float>(tick + 1), row + 0.4f, p);
    int left = bl.x;
    int top = tr.y;
    int width = tr.x - bl.x;
    int height = bl.y - tr.y;

    fillRect(image, left, top, width, height, color);
    outlineRect(image, left, top, width, height, sf::Color(15, 18, 25));

    if (lottery) {
        fillRect(image, left + 3, top + 3, 8, 8, LOTTERY_COLOR);
        outlineRect(image, left + 3, top + 3, 8, 8, sf::Color::Black);
    }
}

void drawReady(sf::Image& image, const PlotArea& p, int tick, int row) {
    sf::Vector2i bl = mapToPixel(static_cast<float>(tick), row - 0.4f, p);
    sf::Vector2i tr = mapToPixel(static_cast<float>(tick + 1), row + 0.4f, p);
    outlineRect(image, bl.x + 1, tr.y + 1, tr.x - bl.x - 2, bl.y - tr.y - 2, READY_BORDER);
}

void drawSuspended(sf::Image& image, const PlotArea& p, int tick, int row,
                   sim::SuspendReason reason) {
    sf::Vector2i bl = mapToPixel(static_cast<float>(tick), row - 0.4f, p);
    sf::Vector2i tr = mapToPixel(static_cast<float>(tick + 1), row + 0.4f, p);
    int w = tr.x - bl.x;
    int h = bl.y - tr.y;

    fillRect(image, bl.x, tr.y, w, h, SUSPENDED_COLOR);
    outlineRect(image, bl.x, tr.y, w, h, sf::Color(80, 80, 80));

    if (reason == sim::SuspendReason::IO) {
        // Listras horizontais azuis (req 3.8).
        for (int y = tr.y + 3; y < bl.y - 3; y += 6) {
            fillRect(image, bl.x + 2, y, w - 4, 2, SUSPENDED_IO_STRIPE);
        }
    } else if (reason == sim::SuspendReason::MUTEX) {
        // Padrao quadriculado vermelho (req 2.9).
        int step = 4;
        for (int oy = 3; oy < h - 3; oy += step) {
            int startOx = ((oy / step) % 2) ? 3 : 3 + step / 2;
            for (int ox = startOx; ox < w - 3; ox += step) {
                fillRect(image, bl.x + ox, tr.y + oy, 2, 2, SUSPENDED_MUTEX_STRIPE);
            }
        }
    }
}

// Marcadores das acoes de ML/MU/IO no canto inferior direito do bloco
// correspondente (req 2.8 / 3.5.1).
void drawActionMarker(sf::Image& image, const PlotArea& p,
                      int tick, int row, const sim::ActionEvent& ev) {
    sf::Vector2i bl = mapToPixel(static_cast<float>(tick), row - 0.4f, p);
    sf::Vector2i tr = mapToPixel(static_cast<float>(tick + 1), row + 0.4f, p);
    int w = tr.x - bl.x;

    int cx = bl.x + w - 10;
    int cy = bl.y - 10;

    switch (ev.type) {
        case sim::ActionType::MUTEX_LOCK:
            fillRect(image, cx, cy, 8, 8, MUTEX_LOCK_COLOR);
            outlineRect(image, cx, cy, 8, 8, sf::Color::Black);
            break;
        case sim::ActionType::MUTEX_UNLOCK:
            fillRect(image, cx, cy, 8, 8, MUTEX_UNLOCK_COLOR);
            outlineRect(image, cx, cy, 8, 8, sf::Color::Black);
            break;
        case sim::ActionType::IO:
            // Losango aproximado por triangulos-linha.
            for (int i = 0; i < 4; ++i) {
                int y = cy + i;
                fillRect(image, cx + (3 - i), y, 2 + 2 * i, 1, IO_ICON_COLOR);
            }
            for (int i = 0; i < 4; ++i) {
                int y = cy + 4 + i;
                fillRect(image, cx + i, y, 8 - 2 * i, 1, IO_ICON_COLOR);
            }
            break;
    }
}

void drawArrival(sf::Image& image, const PlotArea& p, int tick, int row) {
    sf::Vector2i base = mapToPixel(static_cast<float>(tick), row - 0.4f, p);
    for (int y = 0; y < 10; ++y) {
        int half = y / 2;
        for (int x = -half; x <= half; ++x) {
            putPixel(image, base.x + x, base.y - y, ARRIVAL_COLOR);
        }
    }
}

void drawTermination(sf::Image& image, const PlotArea& p, int tick, int row) {
    sf::Vector2i anchor = mapToPixel(static_cast<float>(tick), row + 0.4f, p);
    drawLine(image, anchor.x - 6, anchor.y - 6, anchor.x + 6, anchor.y + 6, TERMINATION_COLOR);
    drawLine(image, anchor.x - 6, anchor.y + 6, anchor.x + 6, anchor.y - 6, TERMINATION_COLOR);
}

void drawLegendSwatch(sf::Image& image, int x, int y, sf::Color color) {
    fillRect(image, x, y, 16, 16, color);
    outlineRect(image, x, y, 16, 16, sf::Color::Black);
}

}  // namespace

bool exportGanttPng(const sim::OperatingSystem& os,
                    const std::vector<sim::Task>& initialTasks,
                    const std::string& filename) {
    const auto& history = os.getSnapshotsHistory();
    if (history.empty()) return false;

    std::map<int, sf::Color> taskColors;
    std::map<int, int> taskRows;
    for (const auto& t : initialTasks) {
        taskColors[t.id] = t.color;
    }

    int row = 1;
    for (const auto& kv : taskColors) {
        taskRows[kv.first] = row++;
    }

    int rowCount = std::max(1, static_cast<int>(taskRows.size()));
    int totalTicks = static_cast<int>(history.size());
    for (const auto& t : os.getTasks()) {
        if (t.finishTime > totalTicks) totalTicks = t.finishTime;
    }
    if (totalTicks <= 0) totalTicks = 1;

    int pxPerTick = 24;
    int imgW = std::max(900, 120 + totalTicks * pxPerTick + 20);
    int imgH = 120 + rowCount * 50;

    sf::Image image;
    image.create(static_cast<unsigned>(imgW), static_cast<unsigned>(imgH), BG_COLOR);

    PlotArea plot = {
        {70, 50, imgW - 100, imgH - 100},
        0.0f, static_cast<float>(totalTicks),
        0.0f, static_cast<float>(rowCount + 1)
    };

    drawGrid(image, plot, totalTicks, rowCount + 1);

    drawLegendSwatch(image, 20,  18, sf::Color(120, 160, 220));  // RUN
    outlineRect(image,     80,  18, 16, 16, READY_BORDER);        // READY
    drawLegendSwatch(image, 130, 18, SUSPENDED_COLOR);            // SUSP
    drawLegendSwatch(image, 180, 18, SUSPENDED_IO_STRIPE);        // SUSP-IO
    drawLegendSwatch(image, 240, 18, SUSPENDED_MUTEX_STRIPE);     // SUSP-MU
    drawLegendSwatch(image, 300, 18, MUTEX_LOCK_COLOR);           // ML
    drawLegendSwatch(image, 350, 18, MUTEX_UNLOCK_COLOR);         // MU
    drawLegendSwatch(image, 400, 18, IO_ICON_COLOR);              // IO
    drawLegendSwatch(image, 450, 18, LOTTERY_COLOR);              // Sorteio
    drawLegendSwatch(image, 510, 18, sf::Color(245, 120, 120));   // CPU OFF

    for (int t = 0; t < static_cast<int>(history.size()); ++t) {
        for (const auto& task : history[t].tasks) {
            auto rowIt = taskRows.find(task.id);
            if (rowIt == taskRows.end()) continue;

            int taskRow = rowIt->second;
            switch (task.state) {
                case sim::TaskState::RUNNING: {
                    sf::Color color = taskColors.count(task.id) ? taskColors[task.id] : sf::Color::White;
                    drawRunBlock(image, plot, t, taskRow, color, task.wonByLottery);
                    break;
                }
                case sim::TaskState::READY:
                    drawReady(image, plot, t, taskRow);
                    break;
                case sim::TaskState::SUSPENDED:
                    drawSuspended(image, plot, t, taskRow, task.suspendReason);
                    break;
                default:
                    break;
            }
        }

        // Marcadores das acoes ML/MU/IO disparadas neste tick.
        for (const auto& ev : history[t].tickActions) {
            auto rowIt = taskRows.find(ev.taskId);
            if (rowIt == taskRows.end()) continue;
            drawActionMarker(image, plot, t, rowIt->second, ev);
        }
    }

    for (const auto& task : os.getTasks()) {
        auto rowIt = taskRows.find(task.id);
        if (rowIt == taskRows.end()) continue;
        if (task.arrivalTime >= 0 && task.arrivalTime <= totalTicks) {
            drawArrival(image, plot, task.arrivalTime, rowIt->second);
        }
        if (task.finishTime >= 0 && task.finishTime <= totalTicks) {
            drawTermination(image, plot, task.finishTime, rowIt->second);
        }
    }

    return image.saveToFile(filename);
}

}  // namespace view
