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

    auto checkContainerHasAllRoomTypes = [](const auto& c) {
        return std::all_of(std::begin(ROOM_TYPES), std::end(ROOM_TYPES), [&c](ERoomType roomType) {
            return c.count(roomType);
        });
    };

    RoomCountInputs[ERoomType::Single] = ui->SingleRoomCount;
    RoomCountInputs[ERoomType::Double] = ui->DoubleRoomCount;
    if (!checkContainerHasAllRoomTypes(RoomCountInputs)) {
        throw std::runtime_error("RoomCountInputs does not contain some room type");
    }

    RoomCostInputs[ERoomType::Single] = ui->SingleRoomCost;
    RoomCostInputs[ERoomType::Double] = ui->DoubleRoomCost;
    if (!checkContainerHasAllRoomTypes(RoomCostInputs)) {
        throw std::runtime_error("RoomCostInputs does not contain some room type");
    }
}

TStartWindow::~TStartWindow() {
    delete ui;
}


void TStartWindow::on_StartEmulate_clicked() {
    InitRoomCounts();
    InitRoomCosts();
    BusyRooms[ERoomType::Single] = 0;
    BusyRooms[ERoomType::Double] = 0;

    TotalProfit = 0;

    HotelStats = std::make_unique<THotelStats>(RoomCounts);

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
    const auto dayBeforeAdd = Clock->GetTime().Day;
    Clock->Add(step);
    const auto dayAfterAdd = Clock->GetTime().Day;
    if (dayBeforeAdd != dayAfterAdd) {
        for (const auto roomType : ROOM_TYPES) {
            const auto totalCount = RoomCounts.at(roomType);
            const auto busyCount = BusyRooms.at(roomType);
            HotelStats->AddRoomsOccupanccy(roomType, dayBeforeAdd, static_cast<double>(busyCount) / totalCount);
        }
    }
    DisplayTime();
}


void TStartWindow::on_StopEmulation_clicked() {
    DisplayStat();
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

void TStartWindow::InitRoomCounts() {
    for (const auto roomType : ROOM_TYPES) {
        const auto* line = RoomCountInputs.at(roomType);
        bool ok = false;
        const auto value = line->text().toUInt(&ok);
        if (!ok) {
            throw std::runtime_error("Room count must be unsigned int");
        }
        RoomCounts[roomType] = value;
    }
}

void TStartWindow::InitRoomCosts() {
    for (const auto roomType : ROOM_TYPES) {
        const auto* line = RoomCostInputs.at(roomType);
        bool ok = false;
        const auto value = line->text().toUInt(&ok);
        if (!ok) {
            throw std::runtime_error("Room cost must be unsigned int");
        }
        RoomCosts[roomType] = value;
    }
}

void TStartWindow::OnBook(const TBooking& booking, bool success) {
    qDebug() << "Book " << static_cast<int>(booking.RoomType)
        << " from " << booking.DayFrom << " to " << booking.DayTo << " success " << success;
    Bookings.push_back({booking, success});
    if (success) {
        HotelStats->AddAcceptedBooking();
    } else {
        HotelStats->AddRejectedBooking();
    }
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

void TStartWindow::DisplayStat() {
    std::stringstream text;
    text << "Всего бронирований: " << HotelStats->GetTotalBookings() << "<br>";
    text << "Из них подтверждено: " << HotelStats->GetAcceptedBookings() << "<br>";
    text << "Загрузка гостиницы:<br>";
    for (const auto roomType : ROOM_TYPES) {
        text << "    " << RoomTypeToString(roomType) << ": " << HotelStats->GetRoomOccupancy(roomType) * 100 << "%<br>";
    }
    text << "    Гостиница в целом:" << HotelStats->GetRoomOccupancy() * 100 << "%<br>";
    ui->ActionView->setHtml(QString::fromStdString(text.str()));
}
