#include "start_window.h"
#include "./ui_start_window.h"
#include <QErrorMessage>
#include <QDebug>

#include "emulator.h"
#include <mutex>
#include <sstream>

namespace  {
    constexpr unsigned HOURS_IN_DAY = 24;

    enum ECase {
        Genitive,
        Accusative
    };

    std::string RussianRoomType(ERoomType roomType, ECase aCase) {
        switch (roomType) {
            case ERoomType::Single:
                if (aCase == ECase::Accusative) {
                    return "одноместную комнату";
                } else {
                    return "одноместной комнаты";
                }
            case ERoomType::Double:
                if (aCase == ECase::Accusative) {
                    return "двухместную комнату";
                } else {
                    return "двухместной комнаты";
                }
            case ERoomType::DoubleWithSofa:
                if (aCase == ECase::Accusative) {
                    return "двухместную комнату с диваном";
                } else {
                    return "двухместной комнаты c диваном";
                }
            case ERoomType::HalfLux:
                if (aCase == ECase::Accusative) {
                    return "полулюкс";
                } else {
                    return "полулюкса";
                }
            case ERoomType::Lux:
                if (aCase == ECase::Accusative) {
                    return "люкс";
                } else {
                    return "люкса";
                }
        }
    }

    std::string BuildBookingEventText(const TBookingEvent& bookingEvent) {
        const std::string bookStr = bookingEvent.Success ? "забронировал" : "не смог забронировать";
        std::stringstream text;
        text << "Гость " << bookingEvent.Booking.UserId << ' ' << bookStr << ' '
            << RussianRoomType(bookingEvent.Booking.RoomType, ECase::Accusative)
            << " с " << bookingEvent.Booking.DayFrom << " по " << bookingEvent.Booking.DayTo;
        return text.str();
    }

    std::string BuildCheckinEventText(const TCheckinEvent& checkinEvent) {
        const std::string checkinStr = checkinEvent.Success ? "заселился" : "не смог заселиться";
        std::stringstream text;
        text << "Гость " << checkinEvent.Booking.UserId << ' ' << checkinStr << " в "
            << RussianRoomType(checkinEvent.Booking.RoomType, ECase::Accusative) << ' '
            << "с " << checkinEvent.Booking.DayFrom << " по " << checkinEvent.Booking.DayTo;
        return text.str();
    }

    std::string BuildCheckoutEventText(const TCheckoutEvent& checkoutEvent) {
        std::stringstream text;
        text << "Гость " << checkoutEvent.Booking.UserId << " выселился из "
            << RussianRoomType(checkoutEvent.Booking.RoomType, ECase::Genitive) << ", счет " << checkoutEvent.Cost << " руб";
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

#define X(Id) RoomCountInputs[ERoomType::Id] = ui->Id##RoomCount;
    ROOMS
#undef X

#define X(Id) RoomCostInputs[ERoomType::Id] = ui->Id##RoomCost;
    ROOMS
#undef X

#define X(Id) OutputsOfBusyRooms[ERoomType::Id] = ui->Busy##Id;
    ROOMS
#undef X

#define X(Id) OutputsOfFreeRooms[ERoomType::Id] = ui->Free##Id;
    ROOMS
#undef X

    connect(&EmulationTimer, &QTimer::timeout, this, &TStartWindow::on_MakeStep_clicked);
}

TStartWindow::~TStartWindow() {
    delete ui;
}


void TStartWindow::on_StartEmulate_clicked() {
    try {
        InitRoomCounts();
        InitRoomCosts();
        for (const auto roomType : ROOM_TYPES) {
            BusyRooms[roomType] = 0;
        }
        TotalProfit = 0;
        HotelStats = std::make_unique<THotelStats>(RoomCounts);

        ui->ActionView->clear();

        Clock = std::make_unique<TClock>();
        BookingSystem = IBookingSystem::Create(RoomCounts, RoomCosts, GetBookingSystemType(), *Clock);
        Emulator = IEmulator::Create({*BookingSystem, *Clock});
        Emulator->AddObserver(*this);

        DisplayRoomCounts();
        DisplayTime();
        DisplayProfit();
    } catch (const std::exception& e) {
        ReportError(e.what());
    }
}

void TStartWindow::on_MakeStep_clicked() {
    if (!Emulator) {
        return;
    }
    try {
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
        if (EmulationTimer.isActive() && dayAfterAdd > GetDaysToEmulate()) {
            on_StopEmulation_clicked();
        }
    } catch (const std::exception& e) {
        ReportError(e.what());
    }
}

void TStartWindow::on_StopEmulation_clicked() {
    EmulationTimer.stop();
    DisplayStat();
}

void TStartWindow::on_StartEmulationAutomatically_clicked() {
    try {
        on_StartEmulate_clicked();
        const auto interval = GetIntervalBetweenStep();
        EmulationTimer.start(interval * 1000);
    } catch (const std::exception& e) {
        ReportError(e.what());
    }
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

unsigned TStartWindow::GetIntervalBetweenStep() const {
    bool ok = false;
    const auto interval = ui->IntervalBetweenSteps->text().toUInt(&ok);
    if (!ok) {
        throw std::runtime_error("Interval between steps must be unsigned int");
    }
    return interval;
}

unsigned TStartWindow::GetDaysToEmulate() const {
    bool ok = false;
    const auto days = ui->NumberOfDaysToEmulate->text().toUInt(&ok);
    if (!ok) {
        return std::numeric_limits<unsigned>::max();
    }
    return days;
}

IBookingSystem::EType TStartWindow::GetBookingSystemType() const {
    const auto type = ui->BookingSystemType->currentText();
    if (type == "Trivial") {
        return IBookingSystem::EType::Trivial;
    } else if (type == "Smart") {
        return IBookingSystem::EType::Smart;
    }
    throw std::runtime_error("Unknown booking system type " + type.toStdString());
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
    Bookings.push_back({booking, success});
    if (success) {
        HotelStats->AddAcceptedBooking();
    } else {
        HotelStats->AddRejectedBooking();
    }
}

void TStartWindow::OnCheckin(const TBooking& booking, bool success) {
    if (success) {
        ++BusyRooms[booking.RoomType];
    }
    Checkins.push_back({booking, success});
    DisplayRoomCounts();
}

void TStartWindow::OnCheckout(const TBooking& booking, TCost cost) {
    --BusyRooms[booking.RoomType];
    TotalProfit += cost;
    Checkouts.push_back({booking, cost});
    DisplayRoomCounts();
    DisplayProfit();
}

void TStartWindow::DisplayRoomCounts() {
    for (const auto roomType : ROOM_TYPES) {
        const auto freeRooms = RoomCounts.at(roomType) - BusyRooms.at(roomType);
        OutputsOfFreeRooms[roomType]->setText(QString::number(freeRooms));
        OutputsOfBusyRooms[roomType]->setText(QString::number(BusyRooms.at(roomType)));
    }
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

    text << "Из них подтверждено: " << HotelStats->GetAcceptedBookings() << "<br>";
    text << "Загрузка гостиницы:<br>";
    for (const auto roomType : ROOM_TYPES) {
        text << "    " << RoomTypeToString(roomType) << ": " << HotelStats->GetRoomOccupancy(roomType) * 100 << "%<br>";
    }
    text << "    Гостиница в целом:" << HotelStats->GetRoomOccupancy() * 100 << "%<br>";
    ui->ActionView->setHtml(QString::fromStdString(text.str()));
}

void TStartWindow::ReportError(const std::string& message) {
    auto* errorMessage = new QErrorMessage(this);
    errorMessage->showMessage(QString::fromStdString(message));
}
