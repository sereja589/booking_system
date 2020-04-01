#include "start_window.h"
#include "./ui_start_window.h"
#include <QDebug>

#include "emulator.h"
#include <mutex>
#include <sstream>

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

TStartWindow::~TStartWindow() {
    delete ui;
}


void TStartWindow::on_StartEmulate_clicked() {
    RoomCosts[ERoomType::Single] = 3000;
    RoomCosts[ERoomType::Double] = 5000;
    RoomCounts[ERoomType::Single] = 6;
    RoomCounts[ERoomType::Double] = 6;
    BusyRooms[ERoomType::Single] = 0;
    BusyRooms[ERoomType::Double] = 0;

    TotalProfit = 0;

    Clock = std::make_unique<TClock>();
    BookingSystem = IBookingSystem::Create(RoomCounts, RoomCosts, IBookingSystem::EType::Trivial, *Clock);
    Emulator = IEmulator::Create({*BookingSystem, *Clock});
    Emulator->AddObserver(*this);
    DisplayRoomCounts();
    DisplayTime();
    DisplayProfit();
}

void TStartWindow::on_MakeStep_clicked() {
    const auto step = GetEmulateStep();
    Emulator->MakeStep();
    Clock->Add(step);
    DisplayTime();
}

unsigned TStartWindow::GetEmulateStep() const {
    bool ok = false;
    auto result = ui->EmulationStep->text().toUInt(&ok);
    if (!ok) {
        throw std::runtime_error("Emulation step must be unsigned int");
    }
    if (result > 24) {
        throw std::runtime_error("Emulation step must be less or equal 24 hours");
    }
    return result;
}

void TStartWindow::OnBook(const TBooking& booking, bool success) {
    qDebug() << "Book " << static_cast<int>(booking.RoomType)
        << " from " << booking.DayFrom << " to " << booking.DayTo << " success " << success;
}

void TStartWindow::OnCheckin(const TBooking& booking, bool success) {
    qDebug() << "Checkin " << static_cast<int>(booking.RoomType)
        << " from " << booking.DayFrom << " to " << booking.DayTo << " success " << success;
    if (success) {
        ++BusyRooms[booking.RoomType];
    }
    DisplayRoomCounts();
}

void TStartWindow::OnCheckout(const TBooking& booking, TCost cost) {
    qDebug("checkout");
    --BusyRooms[booking.RoomType];
    TotalProfit += cost;
    DisplayRoomCounts();
    DisplayProfit();
}

void TStartWindow::DisplayRoomCounts() {
    ui->FreeSingle->setText(QString::number(RoomCounts.at(ERoomType::Single) - BusyRooms.at(ERoomType::Single)));
    ui->FreeDouble->setText(QString::number(RoomCounts.at(ERoomType::Double) - BusyRooms.at(ERoomType::Double)));
    ui->BusySingle->setText(QString::number(BusyRooms.at(ERoomType::Single)));
    ui->BusyDouble->setText(QString::number(BusyRooms.at(ERoomType::Double)));
}

void TStartWindow::DisplayTime() {
    const auto time = Clock->GetTime();
    std::stringstream timeStr;
    timeStr << "Day " << time.Day << ", hour " << time.Hour;
    ui->CurrentTime->setText(QString::fromStdString(timeStr.str()));
}

void TStartWindow::DisplayProfit() {
    ui->TotalHotelProfit->setText(QString::number(TotalProfit) + " руб");
}
