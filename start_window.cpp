#include "start_window.h"
#include "./ui_start_window.h"
#include <QGraphicsTextItem>
#include <QDebug>

#include "emulator.h"
#include <mutex>
#include <sstream>

namespace  {
    constexpr unsigned HOURS_IN_DAY = 24;

    std::string BuildBookingEventText(const TBookingEvent& bookingEvent) {
        const std::string successStr = bookingEvent.Success ? "successfully" : "unsuccessfully";
        std::stringstream text;
        text << "User " << bookingEvent.Booking.UserId << " books room "
            << RoomTypeToString(bookingEvent.Booking.RoomType) << "\n"
            << "from " << bookingEvent.Booking.DayFrom << " to " << bookingEvent.Booking.DayTo << ' ' << successStr;
        return text.str();
    }

    std::string BuildCheckinEventText(const TCheckinEvent& checkinEvent) {
        const std::string successStr = checkinEvent.Success ? "successfully" : "unsuccessfully";
        std::stringstream text;
        text << "User " << checkinEvent.Booking.UserId << " checkin "
            << RoomTypeToString(checkinEvent.Booking.RoomType) << "\n"
            << "from " << checkinEvent.Booking.DayFrom << " to " << checkinEvent.Booking.DayTo << ' ' << successStr;
        return text.str();
    }

    std::string BuildCheckoutEventText(const TCheckoutEvent& checkoutEvent) {
        std::stringstream text;
        text << "User " << checkoutEvent.Booking.UserId << " checkout "
            << RoomTypeToString(checkoutEvent.Booking.RoomType) << ", bill " << checkoutEvent.Cost;
        return text.str();
    }
}

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
    ClearEvents();
    Emulator->MakeStep();
    DisplayLastEvents();
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
    Bookings.push_back({booking, success});
}

void TStartWindow::OnCheckin(const TBooking& booking, bool success) {
    qDebug() << "Checkin " << static_cast<int>(booking.RoomType)
        << " from " << booking.DayFrom << " to " << booking.DayTo << " success " << success;
    if (success) {
        ++BusyRooms[booking.RoomType];
    }
    Checkins.push_back({booking, success});
    DisplayRoomCounts();
}

void TStartWindow::OnCheckout(const TBooking& booking, TCost cost) {
    qDebug("checkout");
    --BusyRooms[booking.RoomType];
    TotalProfit += cost;
    Checkouts.push_back({booking, cost});
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

void TStartWindow::ClearEvents() {
    Bookings.clear();
    Checkins.clear();
    Checkouts.clear();
}

void TStartWindow::DisplayLastEvents() {
    std::vector<std::string> events;
    for (const auto& booking : Bookings) {
        events.push_back("<center><b>" + BuildBookingEventText(booking) + "</b></center>");
    }
    for (const auto& checkin : Checkins) {
        events.push_back("<center><b>" + BuildCheckinEventText(checkin) + "</b></center>");
    }
    for (const auto& checkout : Checkouts) {
        events.push_back("<center><b>" + BuildCheckoutEventText(checkout) + "</b></center>");
    }
    std::stringstream text;
    for (const auto& event : events) {
        text << event << "<br>";
    }
    ui->ActionView->setHtml(QString::fromStdString(text.str()));
}
