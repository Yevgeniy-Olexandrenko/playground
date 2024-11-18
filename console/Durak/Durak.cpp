#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <windows.h> // Для SetConsoleOutputCP
#include <string>
#include <sstream>
#include <iomanip>
#include <set>

// Перечисления для мастей и рангов карт
enum Suit { SUIT_NONE = -1, SUIT_HEARTS = 0, SUIT_DIAMONDS, SUIT_CLUBS, SUIT_SPADES };
enum Rank { RANK_NONE = -1, RANK_SIX = 6, RANK_SEVEN, RANK_EIGHT, RANK_NINE, RANK_TEN, RANK_JACK, RANK_QUEEN, RANK_KING, RANK_ACE };

// Структура для представления карты
struct Card 
{
    Suit suit = SUIT_NONE;
    Rank rank = RANK_NONE;

    operator bool() const { return (suit != SUIT_NONE && rank != RANK_NONE); }
    bool operator== (const Card& other) const { return (suit == other.suit && rank == other.rank); }
    bool operator<  (const Card& other) const { return (rank == other.rank ? suit < other.suit : rank < other.rank); }

    void print() const
    {
        const char* suits[] = { "\u2665", "\u2666", "\u2663", "\u2660" }; // ♥ ♦ ♣ ♠
        const char* ranks[] = { "6", "7", "8", "9", "10", "J", "Q", "K", "A" };
        std::cout << ranks[rank - RANK_SIX] << suits[suit];
    }
};
using Cards = std::vector<Card>;

// Класс для представления колоды карт
class Deck 
{
public:
    Deck()
    {
        // Создаем колоду из 36 карт
        for (int i = 0; i < 36; ++i)
            m_cards.push_back({ Suit(i % 4), Rank(RANK_SIX + i / 4) });

        // Перемешиваем колоду карт
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(m_cards.begin(), m_cards.end(), g);

        // В качестве козыря берем последнюю карту колоды
        m_trump = m_cards.front();
    }

    const Card& getTrumpCard() const { return m_trump; }
    bool isEmpty() const { return m_cards.empty(); }
    Card getNextCard() 
    {
        if (isEmpty()) return Card();
        Card card = m_cards.back();
        m_cards.pop_back();
        return card;
    }

    void print() 
    {
        std::cout << "Колода: ";
        m_trump.print();
        std::cout << " (" << m_cards.size() << ")\n";
    }

private:
    Cards m_cards;
    Card  m_trump;
};

// Класс для представления игрового стола
class Table 
{
public:
    void addAttackCard(const Card& card) { attackCards.push_back(card); }
    void addDefenseCard(const Card& card) { defenseCards.push_back(card); }
    void clear() 
    {
        attackCards.clear();
        defenseCards.clear();
    }

    bool isEmpty() const { return (attackCards.empty()); }
    bool hasCardWithRank(Rank rank) const 
    {
        for (const auto& card : attackCards)
            if (card.rank == rank) return true;
        for (const auto& card : defenseCards)
            if (card.rank == rank) return true;
        return false;
    }

    const Card&  getLastAttackCard() const { return attackCards.back(); }
    const Cards& getAttackCards() const { return attackCards; }
    const Cards& getDefenseCards() const { return defenseCards; }

    void print() const {
        std::cout << "Карты на столе:\n";
        for (size_t i = 0; i < attackCards.size(); ++i)
        {
            std::cout << std::setw(2);
            attackCards[i].print();
            if (i < defenseCards.size())
            {
                std::cout << " X ";
                defenseCards[i].print();
            }
            std::cout << "\n";
        }
    }

private:
    Cards attackCards, defenseCards;
};

// Класс для представления игрока
class Player {
public:
    Player(const std::string& name, bool isAI)
        : name(name), isAI(isAI) {}

    void addCard(const Card& card) {
        hand.insert(card);
    }

    void removeCard(const Card& card) {
        hand.erase(card);
    }

    void showHandForInspect() const {
        std::cout << name << " карты: ";
        for (const auto& card : hand) {
            std::cout << std::setw(2);
            card.print();
            std::cout << ' ';
        }
        std::cout << "\n";
    }

    void showHandForChoose(const std::vector<bool>& highlight = {}) const {
        std::cout << name << " карты: ";
        size_t index = 1;
        for (const auto& card : hand) {
            std::cout << std::setw(2);
            card.print();
            std::cout << (highlight.size() >= index && highlight[index - 1] ? '!' : '.') << index++ << ' ';
        }
        std::cout << "\n";
    }

    std::string getName() const { return name; }
    bool hasCards() const { return !hand.empty(); }

    std::set<Card>& getHand() { return hand; }
    const std::set<Card>& getHand() const { return hand; }

    Card attack(const Card& trumpCard) {
        if (hand.empty())
            return Card();

        Card card = aiChooseAttackCard(trumpCard);
        if (isAI) {
            removeCard(card);
            return card;
        }

        // Для человеческого игрока
        std::vector<bool> highlight(hand.size(), false);
        size_t index = 0;
        for (const auto& handCard : hand) {
            if (handCard == card) {
                highlight[index] = true;
                break;
            }
            ++index;
        }

        showHandForChoose(highlight);
        int cardIndex = -1;
        while (cardIndex < 1 || cardIndex > hand.size()) {
            std::cout << "Выберите карту для атаки (1 - " << hand.size() << "): ";
            std::cin >> cardIndex;
        }

        auto it = hand.begin();
        std::advance(it, cardIndex - 1);
        card = *it;
        removeCard(card);
        return card;
    }

    Card defense(const Card& attackCard, const Card& trumpCard) {
        if (isAI)
        {
            Card card = aiChooseDefenseCard(attackCard, trumpCard);
            if (card)
            {
                removeCard(card);
                return card;
            }
            return Card();
        }

        // Для человеческого игрока
        std::vector<bool> highlight(hand.size(), false);
        size_t index = 0;
        for (const auto& handCard : hand) {
            if (isValidDefense(attackCard, handCard, trumpCard)) {
                highlight[index] = true;
            }
            ++index;
        }

        showHandForChoose(highlight);
        int cardIndex = -1;
        while (cardIndex < 0 || cardIndex > hand.size()) {
            std::cout << "Выберите карту для защиты (1 - " << hand.size() << ", 0 - взять карты): ";
            std::cin >> cardIndex;

            if (cardIndex == 0) return Card();
            if (highlight[cardIndex - 1]) break;

            std::cout << "Эту карту нельзя выбрать, попробуй еще раз!\n";
            cardIndex = -1;
        }

        auto it = hand.begin();
        std::advance(it, cardIndex - 1);
        Card card = *it;
        removeCard(card);
        return card;
    }

    Card additionalAttack(const Table& table, size_t defenderHandSize, const Card& trumpCard) {
        if (hand.empty() || !hasCardsToAdd(table))
            return Card();

        if (isAI) {
            Card card = aiChooseAdditionalAttackCard(table, defenderHandSize, trumpCard);
            if (card) {
                removeCard(card);
                return card;
            }
            return Card();
        }

        // Для человеческого игрока
        std::cout << name << " может подкинуть карты:\n";
        std::vector<bool> highlight(hand.size(), false);
        size_t index = 0;
        for (const auto& handCard : hand) {
            if (table.hasCardWithRank(handCard.rank)) {
                highlight[index] = true;
            }
            ++index;
        }

        showHandForChoose(highlight);
        int cardIndex = -1;
        while (cardIndex < 0 || cardIndex > hand.size()) {
            std::cout << "Выберите карту для подкидывания (1 - " << hand.size() << ", 0 - пропустить): ";
            std::cin >> cardIndex;

            if (cardIndex == 0) return Card();
            if (highlight[cardIndex - 1]) break;

            std::cout << "Эту карту нельзя выбрать, попробуй еще раз!\n";
            cardIndex = -1;
        }

        auto it = hand.begin();
        std::advance(it, cardIndex - 1);
        Card card = *it;
        removeCard(card);
        return card;
    }

private:
    bool isAI;
    std::string name;
    std::set<Card> hand;

    // Проверяет, можно ли использовать карту для атаки
    bool canAttackWith(const Table& table, const Card& card) const {
        return (table.isEmpty() || table.hasCardWithRank(card.rank));
    }

    // Проверяет, можно ли отбиться картой defendCard от attackCard
    bool isValidDefense(const Card& attackCard, const Card& defendCard, const Card& trumpCard) const {
        if (defendCard.suit == attackCard.suit && defendCard.rank > attackCard.rank)
            return true;
        if (defendCard.suit == trumpCard.suit && attackCard.suit != trumpCard.suit)
            return true;
        return false;
    }

    // Общий метод для выбора карты с минимальным "весом"
    template <typename Iterator>
    Card chooseLowestWeightCard(Iterator begin, Iterator end, const Card& trumpCard) const {
        return *std::min_element(begin, end, [&trumpCard](const Card& a, const Card& b) {
            int aWeight = a.rank; if (a.suit == trumpCard.suit) aWeight += RANK_ACE;
            int bWeight = b.rank; if (b.suit == trumpCard.suit) bWeight += RANK_ACE;
            return aWeight < bWeight;
            });
    }

    // Методы ИИ
    Card aiChooseAttackCard(const Card& trumpCard) {
        // Предпочитаем карты низкого ранга и не козырь
        return chooseLowestWeightCard(hand.begin(), hand.end(), trumpCard);
    }

    Card aiChooseDefenseCard(const Card& attackCard, const Card& trumpCard) {
        std::vector<Card> possibleDefenses;

        // Собираем возможные карты для защиты
        for (const auto& card : hand)
        {
            if (isValidDefense(attackCard, card, trumpCard))
            {
                possibleDefenses.push_back(card);
            }
        }

        if (!possibleDefenses.empty()) {
            // Предпочитаем отбиваться картами низкого ранга
            return chooseLowestWeightCard(possibleDefenses.begin(), possibleDefenses.end(), trumpCard);
        }

        // Не может отбиться
        return Card();
    }

    Card aiChooseAdditionalAttackCard(const Table& table, size_t defenderHandSize, const Card& trumpCard) {
        if (defenderHandSize > 1)
        {
            std::vector<Card> possibleAttacks;

            // Собираем возможные карты для атаки
            for (const auto& card : hand) {
                if (canAttackWith(table, card)) {
                    possibleAttacks.push_back(card);
                }
            }

            if (!possibleAttacks.empty()) {
                // Предпочитаем карты низкого ранга и не козырь
                return chooseLowestWeightCard(possibleAttacks.begin(), possibleAttacks.end(), trumpCard);
            }
        }
        return Card();
    }

    // Проверяет, есть ли у игрока карты для подкидывания
    bool hasCardsToAdd(const Table& table) const {
        for (const auto& card : hand) {
            if (canAttackWith(table, card)) return true;
        }
        return false;
    }
};

// Класс для представления игры
class Game 
{
public:
    Game(bool useAIasHuman)
    {
        // Задаем число игроков
        int numPlayers = 0;
        while (numPlayers < 2 || numPlayers > 4)
        {
            std::cout << "Введите количество игроков (2-4): ";
            std::cin >> numPlayers;
        }

        // Создаем игроков
        m_players.push_back(Player("YOU", useAIasHuman));
        for (int i = 1; i < numPlayers; ++i)
        {
            m_players.push_back(Player("AI" + std::to_string(i), true));
        }

        // Раздача карт игрокам
        for (int i = 0; i < 6; ++i)
            for (auto& player : m_players)
                player.addCard(m_deck.getNextCard());

        // Определение первого атакующего игрока
        m_attackerIndex = findFirstAttacker(m_deck.getTrumpCard());
        std::cout << "Первым ходит игрок: " << m_players[m_attackerIndex].getName() << "\n";
    }

    void loop()
    {
        while (!isGameOver())
        {
            if (m_players.size() <= 1)
                break;

            printNewRoundBegin();

            // TODO: something wrong with index after players deletion
            int defenderIndex = (m_attackerIndex + 1) % m_players.size();
            Player& attacker = m_players[m_attackerIndex];
            Player& defender = m_players[defenderIndex];

            // Атака
            Card attackCard = attacker.attack(m_deck.getTrumpCard());
            if (attackCard)
            {
                std::cout << attacker.getName() << " атакует " << defender.getName();
                std::cout << " картой: ";
                attackCard.print();
                std::cout << "\n";
                attacker.showHandForInspect();
            }
            else
            {
                std::cout << attacker.getName() << " пропускает ход.\n";
                m_attackerIndex = (m_attackerIndex + 1) % m_players.size();
                continue;
            }

            m_table.addAttackCard(attackCard);
            m_table.print();

            bool defenderTookCards = false;
            while (true)
            {
                // Защитник пытается отбиться от последней подкинутой карты
                Card defendCard = defender.defense(m_table.getLastAttackCard(), m_deck.getTrumpCard());
                if (defendCard)
                {
                    std::cout << defender.getName() << " отбивается картой: ";
                    defendCard.print();
                    std::cout << "\n";
                    defender.showHandForInspect();

                    m_table.addDefenseCard(defendCard);
                    m_table.print();
                }
                else
                {
                    std::cout << defender.getName() << " не может отбиться, забирает карты.\n";

                    defenderTookCards = true;
                    defenderTakesAllCardsFromTable(defender);
                    break;
                }

                // Проверяем, можно ли подкидывать еще карты
                if (!defender.hasCards()) break;

                // Подкидывание карт
                bool someoneAddedCard = false;
                for (size_t i = 0; i < m_players.size(); ++i)
                {
                    int attackerIndex = (m_attackerIndex + i) % m_players.size();
                    if (attackerIndex == defenderIndex) continue;
                    Player& attacker = m_players[attackerIndex];

                    Card additionalCard = attacker.additionalAttack(m_table, defender.getHand().size(), m_deck.getTrumpCard());
                    if (additionalCard)
                    {
                        std::cout << "\n";
                        std::cout << attacker.getName() << " подкидывает карту: ";
                        additionalCard.print();
                        std::cout << "\n";
                        attacker.showHandForInspect();

                        m_table.addAttackCard(additionalCard);
                        m_table.print();

                        someoneAddedCard = true;
                        break; // Подкидываем по одной карте
                    }
                }
                if (!someoneAddedCard) break;
            }

            if (!defenderTookCards)
            {
                std::cout << defender.getName() << " успешно отбился, карты идут в отбой.\n";
                discardCardsFromTable();
            }

            // Добор карт из колоды
            refillHands();

            // Проверка на выход игроков из игры
            removePlayersWithoutCards();
            if (m_players.size() == 1)
                break;

            // Переход хода
            if (defenderTookCards)
                m_attackerIndex = (defenderIndex + 1) % m_players.size();
            else
                m_attackerIndex = defenderIndex;
        }

        announceWinner();
    }

private:
    void printNewRoundBegin()
    {
        std::cout << '\n';
        std::cout << std::string(10, '-');
        std::cout << " ROUND: " << ++m_roundNumber << ' ';
        std::cout << std::string(10, '-');
        std::cout << '\n';

        // Вывод состояния колоды
        std::cout << "\n";
        m_deck.print();

        // Вывод карт игроков перед ходом
        std::cout << "Карты игроков перед ходом:\n";
        for (const auto& player : m_players)
        {
            player.showHandForInspect();
        }
        std::cout << "\n";
    }

    void discardCardsFromTable()
    {
        // Карты уходят в сброс
        for (const auto& card : m_table.getAttackCards())
            m_discardPile.push_back(card);
        for (const auto& card : m_table.getDefenseCards())
            m_discardPile.push_back(card);
        m_table.clear();
    }

    void defenderTakesAllCardsFromTable(Player& defender)
    {
        // Защитник берет все карты со стола
        for (const auto& card : m_table.getAttackCards())
            defender.addCard(card);
        for (const auto& card : m_table.getDefenseCards())
            defender.addCard(card);
        m_table.clear();
    }

    int findFirstAttacker(const Card& trumpCard) 
    {
        // Ищем игрока с наименьшей козырной картой
        int firstAttackerIndex = -1;
        Card lowestTrump{ trumpCard.suit, RANK_ACE };
        for (size_t i = 0; i < m_players.size(); ++i) 
        {
            for (const auto& card : m_players[i].getHand()) 
            {
                if (card.suit == trumpCard.suit) 
                {
                    if (card.rank < lowestTrump.rank || firstAttackerIndex == -1) 
                    {
                        lowestTrump = card;
                        firstAttackerIndex = static_cast<int>(i);
                    }
                }
            }
        }

        // Если ни у кого нет козырей, выбираем первого игрока случайно
        if (firstAttackerIndex == -1) 
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, m_players.size() - 1);
            firstAttackerIndex = dis(gen);
        }

        return firstAttackerIndex;
    }

    bool isGameOver() const { return m_players.size() <= 1; }

    void refillHands()
    {
        // Порядок добора: атакующий, остальные, защитник
        std::vector<int> refillOrder;
        refillOrder.push_back(m_attackerIndex);
        for (size_t i = 1; i < m_players.size(); ++i)
            refillOrder.push_back((m_attackerIndex + i) % m_players.size());

        for (int idx : refillOrder)
        {
            Player& player = m_players[idx];
            while (player.getHand().size() < 6 && !m_deck.isEmpty())
            {
                player.addCard(m_deck.getNextCard());
            }
        }
    }

    void removePlayersWithoutCards() {
        m_players.erase(remove_if(m_players.begin(), m_players.end(),
            [](const Player& player)
            {
                return !player.hasCards();
            }
        ), m_players.end());
    }

    void announceWinner() 
    {
        if (m_players.empty())
        {
            std::cout << "Игра закончилась вничью!\n";
        }
        else
        {
            std::cout << m_players[0].getName() << " проиграл и остается дураком!\n";
            m_players[0].showHandForInspect();
        }
    }

private:
    Deck  m_deck;
    Table m_table;
    Cards m_discardPile;
    std::vector<Player> m_players;
    int m_attackerIndex = 0;
    int m_roundNumber = 0;
};

int main() 
{
    SetConsoleOutputCP(CP_UTF8);
    Game game(true);
    game.loop();
    return 0;
}
