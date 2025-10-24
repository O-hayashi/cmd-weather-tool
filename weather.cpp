#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

using json = nlohmann::json;

class WeatherData
{
private:
  std::string city;
  double temperature;
  double feels_like;
  int humidity;
  int pressure;
  std::string description;
  std::string main;
  double wind_speed;
  bool valid;

public:
  WeatherData() : temperature(0), feels_like(0), humidity(0),
                  pressure(0), wind_speed(0), valid(false) {}

  void setCity(const std::string &c) { city = c; }
  void setTemperature(double t) { temperature = t; }
  void setFeelsLike(double f) { feels_like = f; }
  void setHumidity(int h) { humidity = h; }
  void setPressure(int p) { pressure = p; }
  void setDescription(const std::string &d) { description = d; }
  void setMain(const std::string &m) { main = m; }
  void setWindSpeed(double w) { wind_speed = w; }
  void setValid(bool v) { valid = v; }

  std::string getCity() const { return city; }
  double getTemperature() const { return temperature; }
  double getFeelsLike() const { return feels_like; }
  int getHumidity() const { return humidity; }
  int getPressure() const { return pressure; }
  std::string getDescription() const { return description; }
  std::string getMain() const { return main; }
  double getWindSpeed() const { return wind_speed; }
  bool isValid() const { return valid; }
};

size_t write_callback(void *contents, size_t size, size_t nmemb, std::string *response)
{
  size_t total_size = size * nmemb;
  response->append((char *)contents, total_size);
  return total_size;
}

void displayWeather(const WeatherData &weather)
{
  if (!weather.isValid())
  {
    std::cout << "\033[1;31m[ERROR] \033[0;37mFailed to get weather data.\033[0m" << std::endl;
    return;
  }

  std::cout << "\033[1;37m\n===============================================" << std::endl;
  std::cout << "\033[1;36m    W E A T H E R   S C A N   v1.0\033[1;37m" << std::endl;
  std::cout << "===============================================\033[0m" << std::endl;

  std::cout << "\033[0;90m>> \033[1;33mLocation\033[0;37m........: " << weather.getCity() << std::endl;
  std::cout << "\033[0;90m-----------------------------------------------" << std::endl;

  std::cout << "\033[0;90m>> \033[0;37mWeather.........: \033[1;36m" << weather.getMain()
            << " (" << weather.getDescription() << ")\033[0m" << std::endl;

  std::cout << "\033[0;90m>> \033[0;37mTemperature.....: \033[1;31m"
            << weather.getTemperature() << "  C\033[0m" << std::endl;

  std::cout << "\033[0;90m>> \033[0;37mFeels Like......: \033[1;35m"
            << weather.getFeelsLike() << "  C\033[0m" << std::endl;

  std::cout << "\033[0;90m>> \033[0;37mHumidity........: \033[1;34m"
            << weather.getHumidity() << " %\033[0m" << std::endl;

  std::cout << "\033[0;90m>> \033[0;37mPressure........: \033[1;33m"
            << weather.getPressure() << " hPa\033[0m" << std::endl;

  std::cout << "\033[0;90m>> \033[0;37mWind Speed......: \033[1;32m"
            << weather.getWindSpeed() << " m/s\033[0m" << std::endl;

  std::cout << "\033[0;90m-----------------------------------------------" << std::endl;
  std::cout << "\033[1;32m[OK] \033[0;37mScan completed successfully.\033[0m" << std::endl;
  std::cout << "\033[1;37m===============================================\033[0m\n"
            << std::endl;
}

WeatherData parseWeatherDataConcise(const std::string &jsonResponse)
{
  WeatherData weather;

  try
  {
    json j = json::parse(jsonResponse);

    weather.setCity(j.value("name", "Unknown City"));
    weather.setTemperature(j["main"].value("temp", 0.0) - 273.15);
    weather.setFeelsLike(j["main"].value("feels_like", 0.0) - 273.15);
    weather.setHumidity(j["main"].value("humidity", 0));
    weather.setPressure(j["main"].value("pressure", 0));
    weather.setMain(j["weather"][0].value("main", "Unknown"));
    weather.setDescription(j["weather"][0].value("description", "No description"));
    weather.setWindSpeed(j["wind"].value("speed", 0.0));
    weather.setValid(true);
  }
  catch (...)
  {
    weather.setValid(false);
  }

  return weather;
}

WeatherData fetchWeatherData(const std::string &cityName, const std::string &API_KEY)
{
  CURL *curl;
  CURLcode res;
  std::string response;
  WeatherData weather;

  std::string url = "https://api.openweathermap.org/data/2.5/weather?q=" + cityName + "&appid=" + API_KEY;

  curl = curl_easy_init();
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    std::cout << "Fetching weather data for " << cityName << "..." << std::endl;

    res = curl_easy_perform(curl);
    if (res == CURLE_OK)
    {
      long response_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

      if (response_code == 200)
      {
        weather = parseWeatherDataConcise(response);
      }
      else if (response_code == 404)
      {
        std::cout << "City not found! Please check the spelling." << std::endl;
      }
      else
      {
        std::cout << "API Error. HTTP Code: " << response_code << std::endl;
        std::cout << "Response: " << response << std::endl;
      }
    }
    else
    {
      std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
    }
    curl_easy_cleanup(curl);
  }

  return weather;
}

int main(int argc, char *argv[])
{
  curl_global_init(CURL_GLOBAL_DEFAULT);

  const char *env_apiKey = getenv("API_KEY");

  if (env_apiKey == nullptr)
  {
    std::cerr << "[ERROR] API key not found in environment variables." << std::endl;
    return 1;
  }

  if (argc < 2)
  {
    std::cerr << "[ERROR] Please provide a city name as a command-line argument." << std::endl;
    return 1;
  }

  std::string apiKey(env_apiKey);
  std::string cityName = argv[1];

  WeatherData weather = fetchWeatherData(cityName, apiKey);
  displayWeather(weather);

  curl_global_cleanup();
  return 0;
}