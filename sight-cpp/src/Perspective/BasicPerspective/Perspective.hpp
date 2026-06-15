class Perspective extends Singular {
    private:
    public:
        Perspective();
        ~Perspective();

        void update(float deltaTime);
        void render();
        void handleInput(GLFWwindow* window);
};