#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include "enemy.h"      // Bao gồm enemy.h (đã bao gồm config.h)
#include "mainmenu.h"   // Bao gồm mainmenu.h (sẽ bao gồm config.h)
#include "life.h"       // Bao gồm life.h

// Forward declaration để tránh phụ thuộc vòng tròn (MainMenu đã bao gồm game.h)
// class MainMenu; // Không cần nếu MainMenu include game.h

class Game {
private:
    // --- Thành viên con trỏ và tham chiếu ---
    SDL_Renderer* renderer; // Con trỏ đến renderer chính
    Enemy* enemy;           // Con trỏ đến đối tượng quản lý kẻ địch
    MainMenu* menu;         // Con trỏ đến đối tượng quản lý menu

    // --- Textures ---
    SDL_Texture* mspaceshipTexture;     // Texture tàu người chơi
    SDL_Texture* pauseButtonTexture;    // Texture nút pause
    SDL_Texture* scoreTexture;          // Texture hiển thị điểm
    SDL_Texture* highscoreTexture;      // Texture hiển thị điểm cao
    SDL_Texture* pausedTexture;         // Texture chữ "Paused"
    SDL_Texture* backToMenuTexture;     // Texture nút "Back to Menu"
    SDL_Texture* restartTexture;        // Texture nút "Restart"
    SDL_Texture* gameOverTextTexture;   // Texture chữ "Game Over"
    SDL_Texture* volumeLabelTexture;    // Texture chữ "Volume" (trong màn hình pause)
    SDL_Texture* giveUpTexture;         // Texture nút "Give Up"

    // --- Trạng thái Game ---
    bool gameOver;          // Cờ báo hiệu game đã kết thúc
    bool paused;            // Cờ báo hiệu game đang tạm dừng
    bool showWarning;       // Cờ báo hiệu đang hiển thị cảnh báo tên lửa nhanh
    bool isFirstFastMissile; // Cờ kiểm tra tên lửa nhanh đầu tiên (có thể không cần nữa)
    bool justStarted;       // Cờ báo hiệu game vừa mới bắt đầu (cho lần update đầu)

    // --- Thời gian ---
    Uint32 startTime;           // Thời điểm bắt đầu game (ms)
    Uint32 pauseStartTime;      // Thời điểm bắt đầu tạm dừng (ms)
    Uint32 totalPausedTime;     // Tổng thời gian đã tạm dừng (ms)
    Uint32 warningStartTime;    // Thời điểm bắt đầu hiển thị cảnh báo (ms)
    Uint32 nextSpawnTime;       // Thời điểm dự kiến spawn wave tiếp theo (ms)
    Uint32 lastMissileSpawnTime;// Thời điểm spawn tên lửa cuối cùng trong wave (ms)

    // --- Điểm số và Waves ---
    int score;                  // Điểm hiện tại
    int missileCount;           // Số lượng tên lửa trong wave hiện tại
    int waveCount;              // Số wave đã hoàn thành
    int wavesUntilIncrease;     // Số wave còn lại trước khi tăng độ khó
    int spawnedMissilesInWave;  // Số tên lửa đã spawn trong wave hiện tại

    // --- Vị trí và Góc ---
    int warningX, warningY;     // Tọa độ hiển thị cảnh báo
    float arcStartAngle;        // Góc bắt đầu hiện tại của khiên (rad)

    // --- Tốc độ ---
    // float defaultMissileSpeed; // Đã chuyển vào config.h
    // float maxMissileSpeed;     // Tính toán dựa trên config.h

    // --- Cài đặt (đồng bộ từ MainMenu) ---
    int volume;         // Mức âm lượng hiện tại (0-100)
    int sensitivity;    // Độ nhạy điều khiển khiên (0-100)

    // --- Giao diện người dùng (UI) trong Game ---
    SDL_Rect chitbox;           // Hitbox của tàu người chơi
    SDL_Rect pauseButton;       // Nút pause
    SDL_Rect backToMenuButton;  // Nút "Back to Menu" (Game Over)
    SDL_Rect restartButton;     // Nút "Restart" (Game Over)
    SDL_Rect giveUpButton;      // Nút "Give Up" (Pause)
    SDL_Rect volumeSlider;      // Thanh trượt âm lượng (Pause)
    SDL_Rect volumeKnob;        // Núm trượt âm lượng (Pause)
    bool isDraggingVolume;      // Cờ báo hiệu đang kéo núm âm lượng

    // --- Đối tượng trong Game ---
    struct Circle { int x, y, r; }; // Cấu trúc hình tròn (cho quỹ đạo, mạng)
    Circle trajectory;              // Vòng tròn quỹ đạo khiên

    std::vector<Life> lives;                // Danh sách mạng sống
    std::vector<Target> targets;            // Danh sách tên lửa thường
    std::vector<Target> fastMissiles;       // Danh sách tên lửa nhanh
    std::vector<SpaceShark> spaceSharks;    // Danh sách cá mập
    std::vector<SharkBullet> sharkBullets;  // Danh sách đạn cá mập

    // --- Phương thức nội bộ ---
    void initTextures();            // Khởi tạo các texture cần thiết cho UI (nút, chữ)
    void updateScoreTexture();      // Cập nhật texture điểm số
    void updateHighscoreTexture();  // Cập nhật texture điểm cao
    void updatePausedTexture();     // Cập nhật texture chữ "Paused"
    void updateGameOverTextTexture();// Cập nhật texture chữ "Game Over"
    void updateVolumeLabelTexture();// Cập nhật texture chữ "Volume"
    void DrawCircle(SDL_Renderer* renderer, const Circle& c); // Vẽ hình tròn
    void DrawArc(SDL_Renderer* renderer, const Circle& c, double startAngle, double arcAngle); // Vẽ cung tròn (khiên)

    // Kiểm tra va chạm (có thể cần tối ưu hóa)
    bool CheckCollisionWithArc(const Target& t);
    bool CheckCollisionWithChitbox(const Target& t);
    bool CheckCollisionWithArc(const SpaceShark& ss);
    bool CheckCollisionWithChitbox(const SpaceShark& ss);
    bool CheckCollisionWithArc(const SharkBullet& sb);
    bool CheckCollisionWithChitbox(const SharkBullet& sb);

    // Xử lý khi mất mạng
    void HandleHit();

public:
    // Hàm khởi tạo
    Game(SDL_Renderer* r, Enemy* e, MainMenu* m);
    // Hàm hủy
    ~Game();

    // Xử lý input
    void handleInput(SDL_Event& event); // Bỏ tham chiếu MainMenu& menu
    // Cập nhật trạng thái game
    void update(float deltaTime);
    // Render game
    void render();
    // Reset trạng thái game
    void reset();
    // Bắt đầu game (thiết lập thời gian)
    void startGame();

    // --- Getters & Setters ---
    bool isGameOver() const { return gameOver; }
    bool isPaused() const { return paused; } // Thêm getter cho paused
    int getVolume() const { return volume; }
    void setVolume(int vol); // Cập nhật cả UI và âm lượng
    int getSensitivity() const { return sensitivity; }
    void setSensitivity(int sens) {
        if (sens >= 0 && sens <= 100) {
            sensitivity = sens;
            // Không cần cập nhật gì thêm ở đây vì sensitivity được dùng trực tiếp trong update()
        }
    }
    // Hàm để MainMenu có thể set lại trạng thái khi thoát khỏi Pause/Game Over
    void setGameStatePlaying() {
         if (paused) { // Chỉ tiếp tục nếu đang pause
            if (pauseStartTime != 0) {
                totalPausedTime += SDL_GetTicks() - pauseStartTime;
            }
            paused = false;
            pauseStartTime = 0;
         }
         // Không làm gì nếu đang game over
    }
     void setGameStatePaused() {
        if (!paused && !gameOver) { // Chỉ pause nếu đang chơi
            pauseStartTime = SDL_GetTicks();
            paused = true;
        }
    }
    void triggerGameOver() { // Hàm để Give Up gọi
        if (!gameOver) {
             gameOver = true;
             // Cập nhật điểm cao ngay lập tức nếu cần
             menu->saveHighscores(score);
             updateScoreTexture();
             updateHighscoreTexture();
             // Đặt lại trạng thái pause nếu đang pause
             if(paused) {
                totalPausedTime += SDL_GetTicks() - pauseStartTime;
                pauseStartTime = 0;
                paused = false;
             }
        }
    }

};

#endif // GAME_H
