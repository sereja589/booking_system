#ifndef TSTARTWINDOW_H
#define TSTARTWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include "emulator.h"
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class TStartWindow; }
QT_END_NAMESPACE

class TClock : public IClock {
public:
    TTime GetTime() const override;

    void Add(unsigned additionalHours);

private:
    std::atomic_uint Hours{0};
};

struct TBookingEvent {
    TBooking Booking;
    bool Success;
};

struct TCheckinEvent {
    TBooking Booking;
    bool Success;
};

struct TCheckoutEvent {
    TBooking Booking;
    TCost Cost;
};

class TStartWindow : public QMainWindow, public IEmulatorObserver
{
    Q_OBJECT

public:
    TStartWindow(QWidget *parent = nullptr);
    ~TStartWindow();

private slots:
    void on_StartEmulate_clicked();
    void on_MakeStep_clicked();

private:
    unsigned GetEmulateStep() const;

    void OnBook(const TBooking& booking, bool success) override;
    void OnCheckin(const TBooking& booking, bool success) override;
    void OnCheckout(const TBooking& booking, TCost cost) override;

    void DisplayRoomCounts();
    void DisplayTime();
    void DisplayProfit();

    void ClearEvents();
    void DisplayLastEvents();

private:
    Ui::TStartWindow* ui;

    TRoomCosts RoomCosts;
    TRoomCounts RoomCounts;
    TRoomCounts BusyRooms;

    TCost TotalProfit = 0;

    std::vector<TBookingEvent> Bookings;
    std::vector<TCheckinEvent> Checkins;
    std::vector<TCheckoutEvent> Checkouts;

    std::unique_ptr<TClock> Clock;
    std::unique_ptr<IBookingSystem> BookingSystem;
    std::unique_ptr<IEmulator> Emulator;
};
#endif // TSTARTWINDOW_H
