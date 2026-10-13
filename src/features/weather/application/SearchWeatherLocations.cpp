#include "SearchWeatherLocations.h"

#include <utility>

namespace stukit::weather {

struct SearchWeatherLocations::State
{
    quint64 generation{0};
    WeatherRequestId activeRequest{InvalidWeatherRequestId};
};

SearchWeatherLocations::SearchWeatherLocations(
    ILocationSearchService &service)
    : m_service(service)
    , m_state(std::make_shared<State>())
{
}

SearchWeatherLocations::~SearchWeatherLocations() { cancel(); }

quint64 SearchWeatherLocations::execute(
    QString query,
    LocationSearchCompletion completion)
{
    cancel();
    const quint64 generation = m_state->generation;
    query = query.trimmed();

    if (query.size() < MinimumQueryLength
        || query.size() > MaximumQueryLength) {
        completion(WeatherServiceError::InvalidInput);
        return generation;
    }

    const std::shared_ptr<State> state = m_state;
    auto sharedCompletion =
        std::make_shared<LocationSearchCompletion>(std::move(completion));
    const WeatherRequestId requestId = m_service.search(
        std::move(query),
        [state, generation, sharedCompletion](
            LocationSearchResult result) mutable {
            if (state->generation != generation) {
                return;
            }
            state->activeRequest = InvalidWeatherRequestId;
            (*sharedCompletion)(std::move(result));
        });

    if (requestId == InvalidWeatherRequestId) {
        if (m_state->generation == generation) {
            (*sharedCompletion)(WeatherServiceError::ServiceUnavailable);
        }
        return generation;
    }

    m_state->activeRequest = requestId;
    return generation;
}

void SearchWeatherLocations::cancel()
{
    ++m_state->generation;
    if (m_state->activeRequest != InvalidWeatherRequestId) {
        m_service.cancel(m_state->activeRequest);
        m_state->activeRequest = InvalidWeatherRequestId;
    }
}

} // namespace stukit::weather
