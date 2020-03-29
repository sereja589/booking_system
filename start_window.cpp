#include "start_window.h"
#include "./ui_start_window.h"

#include "emulator.h"
#include <mutex>

constexpr unsigned HOURS_IN_DAY = 24;

IClock::TTime TClock::GetTime() const {
    const auto value = Hours.load();
    return {value / HOURS_IN_DAY, value % HOURS_IN_DAY};
}

void TClock::Add(unsigned additionalHours) {
    Hours += additionalHours;
}

TStartWindow::TStartWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TStartWindow)
{
    ui->setupUi(this);
}

TStartWindow::~TStartWindow()
{
    delete ui;
}


void TStartWindow::on_StartEmulate_clicked()
{
    TRoomCosts roomCosts;
    roomCosts[ERoomType::Single] = 3000;
    roomCosts[ERoomType::HalfLux] = 5000;
    roomCosts[ERoomType::Lux] = 10000;
    TRoomCounts roomCounts;
    roomCounts[ERoomType::Single] = 10;
    roomCounts[ERoomType::HalfLux] = 6;
    roomCounts[ERoomType::Lux] = 4;

    Clock = std::make_unique<TClock>();
    BookingSystem = IBookingSystem::Create(roomCounts, roomCosts, IBookingSystem::EType::Trivial, *Clock);

}

void TStartWindow::on_MakeStep_clicked()
{
    const auto step = GetEmulateStep();
    Emulator->MakeStep();
    Clock->Add(step);
}

unsigned TStartWindow::GetEmulateStep() const
{
    bool ok = false;
    auto result = ui->EmulationStep->text().toUInt(&ok);
    if (!ok) {
        throw std::runtime_error("Emulation step must be unsigned int");
    }
    return result;
}

void TStartWindow::OnBook(const TBooking &booking, bool success)
{

}

void TStartWindow::OnCheckin(const TBooking &booking, bool success)
{

}

void TStartWindow::OnCheckout(const TBooking &booking, TCost cost)
{

}
