#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <string> // Thêm string header
#include "enemy.h"
#include "mainmenu.h"
#include "life.h"

// --- Khai báo hàm trợ giúp load texture (toàn cục) ---
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);

// --- Khai báo cấu trúc mới ---
struct AllyShip {
    float x, y;         // Vị trí hiện tại
    float speed;        // Tốc độ di chuyển ngang
    bool active;        // Trạng thái hoạt động
    bool droppingHeal;  // Cờ báo hiệu đã thả heal hay chưa
};

struct HealItem {
    float x, y;         // Vị trí hiện tại
    float speed;        // Tốc độ rơi xuống
    bool active;        // Trạng thái hoạt động
};

class Game {
private:
    // --- Con trỏ và Tham chiếu ---
    SDL_Renderer* renderer;
    Enemy* enemy;
    MainMenu* menu;

    // --- Textures ---
    SDL_Texture* mspaceshipTexture;
    SDL_Texture* pauseButtonTexture;
    SDL_Texture* scoreTexture;
    SDL_Texture* highscoreTexture;
    SDL_Texture* pausedTexture;
    SDL_Texture* backToMenuTexture;
    SDL_Texture* restartTexture;
    SDL_Texture* gameOverTextTexture;
    SDL_Texture* volumeLabelTexture;
    SDL_Texture* giveUpTexture;
    SDL_Texture* backgroundTexture; // Texture nền game
    // --- Textures mới ---
    SDL_Texture* allyShipTexture;   // Texture cho tàu đồng minh
    SDL_Texture* healItemTexture;   // Texture cho vật phẩm hồi máu

    // --- Âm thanh ---
    Mix_Chunk* sfxShieldHit;
    Mix_Chunk* sfxPlayerHit;
    Mix_Chunk* sfxGameOver;
    Mix_Chunk* sfxWarning;
    // --- Âm thanh mới ---
    Mix_Chunk* sfxHealCollect;      // Âm thanh khi nhặt hồi máu
    Mix_Music* bgmGame;

    // --- Trạng thái Game ---
    bool gameOver;
    bool paused;
    bool showWarning;
    bool justStarted;

    // --- Thời gian ---
    Uint32 startTime;
    Uint32 pauseStartTime;
    Uint32 totalPausedTime;
    Uint32 warningStartTime;
    Uint32 nextSpawnTime;
    Uint32 lastMissileSpawnTime;
    // --- Thời gian mới ---
    Uint32 lastAllySpawnTime;       // Thời điểm cuối cùng đồng minh xuất hiện

    // --- Điểm số và Waves ---
    int score;
    int missileCount;
    int waveCount;
    int wavesUntilIncrease;
    int spawnedMissilesInWave;

    // --- Vị trí và Góc ---
    int warningX, warningY;
    float arcStartAngle;

    // --- Cài đặt ---
    int volume;
    int sensitivity;

    // --- Giao diện người dùng ---
    SDL_Rect chitbox;
    SDL_Rect pauseButton;
    SDL_Rect backToMenuButton;
    SDL_Rect restartButton;
    SDL_Rect giveUpButton;
    SDL_Rect volumeSlider;
    SDL_Rect volumeKnob;
    bool isDraggingVolume; // Biến này vẫn là private

    // --- Đối tượng trong Game ---
    struct Circle { int x, y, r; };
    Circle trajectory;

    std::vector<Life> lives;
    std::vector<Target> targets;
    std::vector<Target> fastMissiles;
    std::vector<SpaceShark> spaceSharks;
    std::vector<SharkBullet> sharkBullets;
    // --- Đối tượng trong Game mới ---
    std::vector<AllyShip> allies;       // Danh sách các tàu đồng minh
    std::vector<HealItem> healItems;    // Danh sách các vật phẩm hồi máu

    // --- Phương thức nội bộ ---
    void initTextures(); // Khởi tạo texture chữ
    void updateScoreTexture();
    void updateHighscoreTexture();
    void updatePausedTexture();
    void updateGameOverTextTexture();
    void updateVolumeLabelTexture();
    void DrawCircle(SDL_Renderer* renderer, const Circle& c);
    void DrawArc(SDL_Renderer* renderer, const Circle& c, double startAngle, double arcAngle);

    // Kiểm tra va chạm
    bool CheckCollisionWithArc(const Target& t);
    bool CheckCollisionWithChitbox(const Target& t);
    bool CheckCollisionWithArc(const SpaceShark& ss);
    bool CheckCollisionWithChitbox(const SpaceShark& ss);
    bool CheckCollisionWithArc(const SharkBullet& sb);
    bool CheckCollisionWithChitbox(const SharkBullet& sb);
    // --- Va chạm mới ---
    bool CheckCollisionWithChitbox(const HealItem& hi); // Va chạm giữa tàu và heal item

    void HandleHit(); // Xử lý khi người chơi bị bắn trúng
    // --- Xử lý mới ---
    void SpawnAlly(); // Hàm tạo đồng minh mới
    void HandleHealCollection(HealItem& heal); // Hàm xử lý khi nhặt heal


public:
    // Constructor nhận các con trỏ và texture nền
    // --- Cập nhật Constructor để nhận thêm sfxHealCollect ---
    Game(SDL_Renderer* r, Enemy* e, MainMenu* m,
         Mix_Chunk* sfxShieldHit, Mix_Chunk* sfxPlayerHit,
         Mix_Chunk* sfxGameOver, Mix_Chunk* sfxWarning,
         Mix_Chunk* sfxHealCollect, // Thêm âm thanh heal
         Mix_Music* bgmGame,
         SDL_Texture* bgTexture);
    ~Game();

    // Các hàm chính
    void handleInput(SDL_Event& event);
    void update(float deltaTime);
    void render();
    void reset();
    void startGame();

    // --- Getters & Setters ---
    bool isGameOver() const { return gameOver; }
    bool isPaused() const { return paused; }
    int getVolume() const { return volume; }
    void setVolume(int vol);
    int getSensitivity() const { return sensitivity; }
    void setSensitivity(int sens);
    // --- THÊM GETTER MỚI ---
    bool isDraggingVolumeSlider() const; // Hàm getter cho isDraggingVolume

    // Hàm cho MainMenu quản lý trạng thái
    void setGameStatePlaying();
    void setGameStatePaused();
    void triggerGameOver();

};

#endif // GAME_H
