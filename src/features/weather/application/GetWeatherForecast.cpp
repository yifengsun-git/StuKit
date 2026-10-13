#include "GetWeatherForecast.h"

#include <utility>

namespace stukit::weather {

struct GetWeatherForecast::State
{
    quint64 generation{0};
    WeatherRequestId activeRequest{InvalidWeatherRequestId};
};

GetWeatherForecast::GetWeatherForecast(IWeatherForecastService &service)
    : m_service(service)
    , m_state(std::make_shared<State>())
{
}

GetWeatherForecast::~GetWeatherForecast() { cancel(); }

quint64 GetWeatherForecast::execute(
    WeatherLocation location,
    ForecastCompletion completion)
{
    cancel();
    const quint64 generation = m_state->generation;
    const std::shared_ptr<State> state = m_state;
    auto sharedCompletion =
        std::make_shared<ForecastCompletion>(std::move(completion));
    const WeatherRequestId requestId = m_service.fetch(
        std::move(location),
        [state, generation, sharedCompletion](
            ForecastResult result) mutable {
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

void GetWeatherForecast::cancel()
{
    ++m_state->generation;
    if (m_state->activeRequest != InvalidWeatherRequestId) {
        m_service.cancel(m_state->activeRequest);
        m_state->activeRequest = InvalidWeatherRequestId;
    }
}

} // namespace stukit::weather
