#include "seafile-applet.h"
#include "account-mgr.h"
#include "api/requests.h"
#include "events-service.h"

namespace {

const int kPerPage = 25;
} // namespace

EventsService* EventsService::singleton_;

EventsService* EventsService::instance()
{
    if (singleton_ == NULL) {
        static EventsService instance;
        singleton_ = &instance;
    }

    return singleton_;
}

EventsService::EventsService(QObject *parent)
    : QObject(parent)
{
    bool is_support_new_file_activities_api = false;
    const Account account = seafApplet->accountManager()->currentAccount();
    // if server version is least version 7.0.0 enable new api
    if (account.isValid()){
        is_support_new_file_activities_api = account.isAtLeastVersion(6, 3, 1);
    }

    if (!is_support_new_file_activities_api) {
        get_events_req_ = NULL;
        more_offset_ = -1;
    } else {
        get_file_activities_req_ = NULL;
        next_ = 1;
    }
    in_refresh_ = false;

}

void EventsService::start()
{
}

void EventsService::stop()
{
}

void EventsService::refresh()
{
    if (seafApplet->accountManager()->currentAccount().isPro()) {
        sendRequest(false);
    }
}

void EventsService::sendRequest(bool is_load_more)
{
    if (in_refresh_) {
        return;
    }

    bool is_support_new_file_activities_api = false;
    const Account& account = seafApplet->accountManager()->currentAccount();
    if (!account.isValid()) {
        in_refresh_ = false;
        return;
    }
    is_support_new_file_activities_api = account.isAtLeastVersion(6, 3, 1);

    in_refresh_ = true;

    if (!is_support_new_file_activities_api) {
        if (get_events_req_) {
            get_events_req_->deleteLater();
        }
    } else {
        if (get_file_activities_req_) {
            get_file_activities_req_->deleteLater();
        }
    }

    if (!is_support_new_file_activities_api) {
        if (!is_load_more) {
            events_.clear();
            more_offset_ = -1;
        }
    } else {
        if (!is_load_more) {
            events_.clear();
            next_ = 1;
        } else {
            ++next_;
        }

    }

    if (!is_support_new_file_activities_api) {
        get_events_req_ = new GetEventsRequest(account, more_offset_);

        connect(get_events_req_, SIGNAL(success(const std::vector<SeafEvent>&, int)),
                this, SLOT(onRefreshSuccess(const std::vector<SeafEvent>&, int)));

        connect(get_events_req_, SIGNAL(failed(const ApiError&)),
                this, SLOT(onRefreshFailed(const ApiError&)));

        get_events_req_->send();
    } else {
        get_file_activities_req_ = new GetFileActivitiesRequest(account, next_);

        connect(get_file_activities_req_, SIGNAL(success(const std::vector<SeafEvent>&)),
                this, SLOT(onNewFileActivitiesRefreshSuccess(const std::vector<SeafEvent>&)));

        connect(get_file_activities_req_, SIGNAL(failed(const ApiError&)),
                this, SLOT(onRefreshFailed(const ApiError&)));

        get_file_activities_req_->send();

    }

}

void EventsService::loadMore()
{
    sendRequest(true);
}

void EventsService::onRefreshSuccess(const std::vector<SeafEvent>& events, int new_offset)
{
    in_refresh_ = false;

    const std::vector<SeafEvent> new_events = handleEventsOffset(events);

    bool is_loading_more = more_offset_ > 0;
    bool has_more = new_offset > 0;
    more_offset_ = new_offset;

    emit refreshSuccess(new_events, is_loading_more, has_more);
}

void EventsService::onNewFileActivitiesRefreshSuccess(const std::vector<SeafEvent>& events)
{
    in_refresh_ = false;

    const std::vector<SeafEvent> new_events = handleEventsOffset(events);

    bool is_loading_more = false;
    bool has_more = false;
    int event_size = events.size();

    if (event_size < kPerPage) {
        is_loading_more = next_ > 1;
        has_more = false;
    } else if (event_size == kPerPage) {
        is_loading_more = next_ > 1;
        has_more = true;
    } else {
        qWarning("incorrect event numbers: %d received from server", event_size);
    }

    emit refreshSuccess(new_events, is_loading_more, has_more);
}

// We use the "offset" param as the starting point of loading more events, but
// if there are new events on the server, the offset would be inaccurate.
const std::vector<SeafEvent>
EventsService::handleEventsOffset(const std::vector<SeafEvent>& new_events)
{
    if (events_.empty()) {
        events_ = new_events;
        return events_;
    }

    const SeafEvent& last = events_[events_.size() - 1];

    int i = 0, n = new_events.size();

    for (i = 0; i < n; i++) {
        const SeafEvent& event = new_events[i];
        if (event.timestamp < last.timestamp) {
            break;
        } else if (event.commit_id == last.commit_id) {
            continue;
        } else {
            continue;
        }
    }

    std::vector<SeafEvent> ret;

    while (i < n) {
        SeafEvent event = new_events[i++];
        events_.push_back(event);
        ret.push_back(event);
    }

    return ret;
}

void EventsService::onRefreshFailed(const ApiError& error)
{
    in_refresh_ = false;

    emit refreshFailed(error);
}

void EventsService::refresh(bool force)
{
    if (force) {
        in_refresh_ = false;
    }

    refresh();
}
