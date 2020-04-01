#ifndef TSTARTWINDOW_H
#define TSTARTWINDOW_H

#include <QMainWindow>
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

private:
    Ui::TStartWindow* ui;
    std::unique_ptr<TClock> Clock;
    TRoomCosts RoomCosts;
    TRoomCounts RoomCounts;
    TRoomCounts BusyRooms;
    TCost TotalProfit = 0;
    std::unique_ptr<IBookingSystem> BookingSystem;
    std::unique_ptr<IEmulator> Emulator;
};
#endif // TSTARTWINDOW_H
