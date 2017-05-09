//////////////////////////////////////////////////////////////////////////////
//
// Code Rain (https://github.com/Hapaxia/CodeRain)
//
// MIT License
//
// Copyright (c) 2017 M.J.Silk
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////////

#define CODERAIN_FULLSCREEN
//#define CODERAIN_DOUBLE_WINDOW_SIZE

#include <SelbaWard.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <assert.h>
#include <random>
#include <functional>

namespace
{

const unsigned int minStripLength{ 10u };
const unsigned int maxStripLength{ 75u };
const float minStripSpeed{ 10.f };
const float maxStripSpeed{ 50.f };
const unsigned int headLength{ 5u };
const unsigned int tailLength{ 4u };
const unsigned int tailExtensionLength{ 3u };
const sf::Color headColor{ sf::Color::White };
const sf::Color baseColor{ sf::Color(0, 153, 0) };
const sf::Color backgroundColor{ sf::Color::Black };
const unsigned int numberOfStrips{ 74u };
const unsigned int numberOfSpinners{ 100u };

std::mt19937 randomGenerator;

inline void randomSeed()
{
	std::random_device rd;
	randomGenerator.seed(rd());
}

inline unsigned int randomValue(const unsigned int min, const unsigned int max)
{
	assert(max > min);
	return std::uniform_int_distribution<unsigned int>{ min, max }(randomGenerator);
}

inline float randomValue(const float min, const float max)
{
	assert(max > min);
	return std::uniform_real_distribution<float>{ min, max }(randomGenerator);
}

char getRandomChar()
{
	unsigned int value{ 19u };
	while (value == 19u)
		value = randomValue(0u, 54u);
	return static_cast<char>(value);
}

inline sf::Uint8 linearInterpolation(const sf::Uint8 start, const sf::Uint8 end, float alpha)
{
	return static_cast<sf::Uint8>(start * (1.f - alpha) + end * alpha);
}

inline sf::Color linearInterpolation(const sf::Color& start, const sf::Color& end, float alpha)
{
	return
	{
		linearInterpolation(start.r, end.r, alpha),
		linearInterpolation(start.g, end.g, alpha),
		linearInterpolation(start.b, end.b, alpha),
		linearInterpolation(start.a, end.a, alpha)
	};
}

inline bool isValidRow(Cs& cs, int row)
{
	return (!((row < 0) || (row >= static_cast<int>(cs.getMode().y))));
}

} // namespace

class Strip
{
public:
	unsigned int x;
	float y;
	unsigned int length;
	float speed;
	bool headNeedsUpdating;
	sf::Vector2u max;

	Strip(const std::vector<Strip>& strips, const sf::Vector2u newMax, const int head = 0)
		: max{ newMax }
		, m_head{ -1 }
		, headNeedsUpdating{ false }
	{
		priv_spawn(strips, head);
	}

	void update(const std::vector<Strip>& strips, const float dt)
	{
		y += speed * dt;
		if (y > static_cast<float>(max.y + length + 1))
			priv_spawn(strips, 0);
		priv_updateHead();
	}

	sf::Vector2i getHeadPosition()
	{
		headNeedsUpdating = false;
		return{ static_cast<int>(x), m_head };
	}

private:
	int m_head;

	void priv_updateHead()
	{
		const int newHead{ static_cast<int>(y) };
		headNeedsUpdating = newHead != m_head;
		m_head = newHead;
	}

	void priv_spawn(const std::vector<Strip>& strips, const int head)
	{
		length = randomValue(minStripLength, maxStripLength);
		if (head < 0)
		{
			m_head = head;
			y = head - 1.f;
		}
		else
			y = -1.f - randomValue(0u, max.y);
		unsigned int column;
		bool columnTaken{ true };
		while (columnTaken)
		{
			speed = randomValue(minStripSpeed, maxStripSpeed);
			column = randomValue(0u, max.x);
			columnTaken = false;
			for (auto& strip : strips)
			{
				if ((column == strip.x) && ((speed >= strip.speed) || (y >= strip.y - strip.length - tailExtensionLength)))
					columnTaken = true;
			}
		}
		x = column;
	}
};

struct Spinner
{
	unsigned int row;
	unsigned int strip;
};

void updateConsoleScreen(Cs& cs, std::vector<Strip>& strips, std::vector<Spinner>& spinners)
{
	for (auto& strip : strips)
	{
		if (strip.headNeedsUpdating)
		{
			const sf::Vector2i headPosition{ strip.getHeadPosition() };
			const int tailRow{ headPosition.y - static_cast<int>(strip.length) };
			if (isValidRow(cs, headPosition.y))
				cs << Cs::Location(headPosition) << (Cs::Affect::Value | Cs::Affect::FgColor) <<  headColor << Cs::Char(getRandomChar());
			cs << Cs::Affect::FgColor;
			for (unsigned int y{ 1u }; y <= headLength; ++y)
			{
				const float ratio{ static_cast<float>(y) / headLength };
				const int targetRow{ headPosition.y - static_cast<int>(y) };
				if (!isValidRow(cs, targetRow))
					continue;
				cs << Cs::Location(headPosition.x, targetRow) << linearInterpolation(headColor, baseColor, ratio) << Cs::Wipe(1);
			}
			for (unsigned int y{ 0u }; y < tailLength; ++y)
			{
				const float ratio{ static_cast<float>(y) / tailLength };
				const int targetRow{ headPosition.y - static_cast<int>(strip.length - y) };
				if (!isValidRow(cs, targetRow))
					continue;
				cs << Cs::Location(headPosition.x, targetRow) << linearInterpolation(backgroundColor, baseColor, ratio) << Cs::Wipe(1);
			}
			for (unsigned int y{ 0u }; y < tailExtensionLength; ++y)
			{
				const int targetRow{ headPosition.y - static_cast<int>(strip.length + y) };
				if (!isValidRow(cs, targetRow))
					continue;
				cs << Cs::Location(headPosition.x, targetRow) << backgroundColor << Cs::Wipe(1);
			}
		}
	}
	cs << Cs::Affect::Value;
	for (auto& spinner : spinners)
	{
		Strip& currentStrip{ strips[spinner.strip] };
		if (static_cast<int>(spinner.row) < currentStrip.getHeadPosition().y - static_cast<int>(currentStrip.length))
		{
			spinner.strip = randomValue(0u, strips.size() - 1);
			spinner.row = randomValue(0u, cs.getMode().y - 1);
		}
		else if (static_cast<int>(spinner.row) < currentStrip.getHeadPosition().y)
			cs << Cs::Location(currentStrip.x, spinner.row) << Cs::Char(getRandomChar());
	}
	cs << Cs::Affect::Default;
}

int main()
{
	randomSeed();

	const sf::Vector2u windowSize{ 480u, 270u };
	const float windowViewMultiplier{ 2.f };
#ifdef CODERAIN_DOUBLE_WINDOW_SIZE
	const sf::Vector2u actualWindowSize(sf::Vector2f(windowSize) * windowViewMultiplier * 2.f);
#else // CODERAIN_DOUBLE_WINDOW_SIZE
	const sf::Vector2u actualWindowSize(sf::Vector2f(windowSize) * windowViewMultiplier);
#endif // CODERAIN_DOUBLE_WINDOW_SIZE

#ifdef CODERAIN_FULLSCREEN
	sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Code Rain", sf::Style::None);
#else // CODERAIN_FULLSCREEN
	sf::RenderWindow window(sf::VideoMode(actualWindowSize.x, actualWindowSize.y), "Code Rain", sf::Style::Default);
#endif // CODERAIN_FULLSCREEN
	sf::View view;
	view.setSize(sf::Vector2f(windowSize));
	view.setCenter(sf::Vector2f(windowSize / 2u));

	sf::Texture texture;
	if (!texture.loadFromFile("texture.png"))
		return EXIT_FAILURE;

	Cs cs;
	cs.setMode({ 74, 50 });
	cs.setTexture(texture);
	cs.setNumberOfTextureTilesPerRow(10);
	cs.setTextureOffset({ 20, 6 });
	cs.setTextureTileSize({ 26, 22 });
	cs.setSize(cs.getPerfectSize() / 4.f);
	cs.setOrigin({ cs.getSize().x / 2.f, 0.f });
	cs.setPosition({ static_cast<float>(windowSize.x) / 2.f, 0.f });
	cs.setShowCursor(false);
	cs.setScrollAutomatically(false);
	cs.setShowBackground(false);
	cs.loadPalette(Cs::Palette::Colors216Web);
	cs.fill(Cs::Cell{ 19, Cs::ColorPair(15, 0) }); // clear cell in matrix texture
	cs << Cs::Fg(12);

	unsigned int stripHeadStart{ 0u };
	std::vector<Strip> strips;
	for (unsigned int i{ 0u }; i < numberOfStrips; ++i)
		strips.emplace_back(strips, cs.getMode() - sf::Vector2u(1, 1), -1 - (stripHeadStart += 3));

	std::vector<Spinner> spinners;
	spinners.resize(numberOfSpinners);
	for (auto& spinner : spinners)
	{
		spinner.row = randomValue(0u, cs.getMode().y - 1);
		spinner.strip = randomValue(0u, strips.size() - 1);
	}

	sf::Time previousTime{ sf::Time::Zero };
	sf::Clock clock;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				switch (event.key.code)
				{
				case sf::Keyboard::Escape:
					window.close();
					break;
				}
				break;
			}
		}

		const sf::Time currentTime{ clock.getElapsedTime() };
		const sf::Time frameTime{ currentTime - previousTime };
		previousTime = currentTime;
		const float dt{ frameTime.asSeconds() };

		for (auto& strip : strips)
			strip.update(strips, dt);

		updateConsoleScreen(cs, strips, spinners);

		window.clear(backgroundColor);
		window.setView(view);
		window.draw(cs);
		window.display();
	}

	return EXIT_SUCCESS;
}
