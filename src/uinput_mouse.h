// Virtual relative mouse backed by /dev/uinput.
#pragma once

class UinputMouse {
public:
    UinputMouse() = default;
    ~UinputMouse();
    UinputMouse(const UinputMouse&) = delete;
    UinputMouse& operator=(const UinputMouse&) = delete;

    bool open(const char* name = "FrameMouse");
    void close();

    void move(int dx, int dy);
    void button(int code, bool pressed);  // BTN_LEFT / BTN_RIGHT
    void sync();

private:
    int fd_ = -1;
    void emit(int type, int code, int value);
};
