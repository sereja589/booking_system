#pragma once

class IClock {
public:
    struct TTime {
        unsigned Day = 0;
        unsigned Hour = 0;
    };

public:
    virtual ~IClock() = default;

    virtual TTime GetTime() const = 0;
};
