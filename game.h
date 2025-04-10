#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "target.h"
#include "life.h"

#define PI M_PI

struct Circle { int x, y, r; };

class Game {
public:
    SDL_Renderer* renderer;
    SDL_Texture* missileTexture;
    SDL_Texture* gameOverTexture; // Texture cho màn hình Game Over
    SDL_Texture* pauseTexture;    // Texture cho màn hình Pause
    SDL_Texture* pauseButtonTexture; // Texture cho nút pause
    SDL_Texture* scoreTexture;    // Texture để hiển thị điểm số
    SDL_Texture* highscoreTexture; // Texture để hiển thị điểm cao nhất
    TTF_Font* font;               // Font để vẽ văn bản
    Circle trajectory = {400, 300, 50};        // Vòng tròn quỹ đạo (nhân vật)
    SDL_Rect chitbox = {390, 270, 20, 60};     // Hitbox của nhân vật
    SDL_Rect pauseButton = {10, 10, 40, 40};   // Hitbox của nút pause (ô vuông 40x40, ở góc trên cùng bên trái)
    std::vector<Target> targets;               // Danh sách đạn địch
    std::vector<Life> lives = {{700, 550, false}, {730, 550, false}, {760, 550, false}}; // Mạng sống
    double arcStartAngle = -PI / 10.3;         // Góc bắt đầu của cung (radian)
    bool gameOver = false;
    bool paused = false;                       // Trạng thái pause
    Uint32 startTime;                          // Thời gian bắt đầu game
    int missileCount = 1;                      // Số lượng tên lửa mỗi đợt
    int waveCount = 0;                         // Đếm số wave đã hoàn thành
    Uint32 nextSpawnTime = 2000;               // Mốc thời gian sinh tên lửa đầu tiên (2 giây)
    Uint32 lastMissileSpawnTime = 0;           // Thời gian sinh tên lửa cuối cùng trong dãy
    int spawnedInWave = 0;                     // Số tên lửa đã sinh trong đợt hiện tại
    float missileSpeed = 150.0f;               // Tốc độ ban đầu của tên lửa
    int score = 0;                             // Điểm số hiện tại
    int highscore = 0;                         // Điểm cao nhất

    Game(SDL_Renderer* r, SDL_Texture* mt);
    ~Game();                                   // Destructor để giải phóng font
    void handleInput(SDL_Event& event);        // Xử lý input
    void update(float deltaTime);
    void render();
    void reset();
    void updateScoreTexture();                 // Cập nhật texture cho điểm số
    void updateHighscoreTexture();             // Cập nhật texture cho điểm cao nhất

private:
    bool CheckCollisionWithArc(Target& t);     // Kiểm tra va chạm với cung
    bool CheckCollisionWithChitbox(Target& t); // Kiểm tra va chạm với hitbox
    void DrawCircle(SDL_Renderer* renderer, Circle& c); // Vẽ vòng tròn
    void DrawArc(SDL_Renderer* renderer, Circle& c, double startAngle, double arcAngle); // Vẽ cung
};

#endif
