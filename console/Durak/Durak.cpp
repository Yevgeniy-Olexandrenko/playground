﻿#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <set>

enum Suit { SUIT_NONE = -1, SUIT_HEARTS = 0, SUIT_DIAMONDS, SUIT_CLUBS, SUIT_SPADES };
enum Rank { RANK_NONE = -1, RANK_SIX = 6, RANK_SEVEN, RANK_EIGHT, RANK_NINE, RANK_TEN, RANK_JACK, RANK_QUEEN, RANK_KING, RANK_ACE };

struct Card 
{
    Rank rank = RANK_NONE;
    Suit suit = SUIT_NONE;

    operator bool() const { return (rank != RANK_NONE && suit != SUIT_NONE); }
    bool operator== (const Card& other) const { return (rank == other.rank && suit == other.suit); }
    bool operator<  (const Card& other) const { return (rank == other.rank ? suit < other.suit : rank < other.rank); }

    void print() const
    {
        const char* suits[] = { "\u2665", "\u2666", "\u2663", "\u2660" }; // ♥ ♦ ♣ ♠
        const char* ranks[] = { "6", "7", "8", "9", "10", "J", "Q", "K", "A" };
        std::cout << ranks[rank - RANK_SIX] << suits[suit];
    }
};

using Cards = std::vector<Card>;
static const Card NothingCard;

class Hand
{
public:
    using Cards = std::set<Card>;

    bool empty() const { return m_cards.empty(); }
    size_t size() const { return m_cards.size(); }

    void give(const Card& card) { if (card) m_cards.insert(card); }
    void take(const Card& card) { if (card) m_cards.erase(card); }
    Card take(size_t i)
    {
        Card chosen = card(i); take(chosen);
        return chosen;
    }

    const Card& card(size_t i) const 
    {
        if (i < size())
        {
            auto it = m_cards.begin();
            std::advance(it, i);
            return *it;
        }
        return NothingCard;
    }

    void print() const
    {
        for (const auto& card : m_cards)
        {
            std::cout << std::setw(2); card.print(); std::cout << ' ';
        }
        std::cout << "\n";
    }

private:
    Cards m_cards;
};

class Deck
{
public:
    Deck()
    {
        for (int i = 0; i < 36; ++i)
            m_cards.push_back({ Rank(RANK_SIX + i / 4), Suit(i % 4) });
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(m_cards.begin(), m_cards.end(), g);
        m_trump = m_cards.front();
    }

    const Card& trump() const { return m_trump; }
    bool empty() const { return m_cards.empty(); }
    Card take()
    {
        if (empty()) return NothingCard;
        Card card = m_cards.back();
        m_cards.pop_back();
        return card;
    }

    void print() const
    {
        m_trump.print();
        std::cout << ' ' << m_cards.size() << '\n';
    }

private:
    Cards m_cards;
    Card  m_trump;
};

class Table
{
public:
    bool empty() const { return m_cards.empty(); }
    size_t size() const { return m_cards.size(); }

    void place(const Card& card) { if (card) m_cards.push_back(card); }
    const Card& card(size_t i) const { return i < size() ? m_cards[i] : NothingCard; }
    const Card& last() const { return m_cards.empty() ? NothingCard : m_cards.back(); }
    Deck& deck() { return m_deck; }

    void give(Hand& hand)
    {
        for (const Card& card : m_cards) hand.give(card);
        m_cards.clear();
    }

    void discard()
    {
        for (const Card& card : m_cards) m_discard.push_back(card);
        m_cards.clear();
    }

    void print() const
    {
        for (size_t i = 0; i < m_cards.size(); ++i)
        {
            std::cout << std::setw(2);
            m_cards[i++].print();
            if (i < m_cards.size())
            {
                std::cout << " X ";
                m_cards[i].print();
            }
            std::cout << '\n';
        }
    }

private:
    Cards m_cards;
    Cards m_discard;
    Deck  m_deck;
};

// Класс для представления игрока
class Player {
public:
    Player(const std::string& name, bool isAI)
        : name(name), isAI(isAI) {}

    void addCard(const Card& card) 
    {
        m_hand.give(card);
    }

    void showHandForInspect() const 
    {
        std::cout << name << ": ";
        m_hand.print();
    }

    void showHandForChoose(const std::vector<bool>& highlight) const 
    {
        showHandForInspect();
        std::cout << name << ": ";
        for (size_t i = 0; i < highlight.size(); ++i)
        {
            std::cout << std::setw(2);
            std::cout << '^' << char(highlight[i] ? ('1' + i) : '.') << ' ';
        }
        std::cout << '\n';
    }

    std::string getName() const { return name; }
    bool hasCards() const { return !m_hand.empty(); }
    Hand& getHand() { return m_hand; }

    Card attack(const Card& trumpCard) 
    {
        if (m_hand.empty())
            return Card();

        if (isAI) 
        {
            Card card = aiChooseAttackCard(trumpCard);
            m_hand.take(card);
            return card;
        }

        // Для человеческого игрока
        std::vector<bool> highlight(m_hand.size(), true);
        showHandForChoose(highlight);

        int cardIndex = -1;
        while (cardIndex < 1 || cardIndex > m_hand.size()) {
            std::cout << "Выберите карту для атаки (1 - " << m_hand.size() << "): ";
            std::cin >> cardIndex;
        }

        Card card = m_hand.take(cardIndex - 1);
        return card;
    }

    Card defense(const Card& attackCard, const Card& trumpCard) {
        if (isAI)
        {
            Card card = aiChooseDefenseCard(attackCard, trumpCard);
            m_hand.take(card);
            return card;
        }

        // Для человеческого игрока
        std::vector<bool> highlight(m_hand.size(), false);
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            if (isValidDefense(attackCard, m_hand.card(i), trumpCard))
            {
                highlight[i] = true;
            }
        }
        showHandForChoose(highlight);

        int cardIndex = -1;
        while (cardIndex < 0 || cardIndex > m_hand.size()) {
            std::cout << "Выберите карту для защиты (1 - " << m_hand.size() << ", 0 - взять карты): ";
            std::cin >> cardIndex;

            if (cardIndex == 0) return Card();
            if (highlight[cardIndex - 1]) break;

            std::cout << "Эту карту нельзя выбрать, попробуй еще раз!\n";
            cardIndex = -1;
        }

        Card card = m_hand.take(cardIndex - 1);
        return card;
    }

    Card additionalAttack(const Table& table, size_t defenderHandSize, const Card& trumpCard) {
        if (m_hand.empty() || !hasValidAttacks(table))
            return NothingCard;

        if (isAI) 
        {
            Card card = aiChooseAdditionalAttackCard(table, defenderHandSize, trumpCard);
            m_hand.take(card);
            return card;
        }

        // Для человеческого игрока
        std::cout << name << " может подкинуть карты:\n";

        std::vector<bool> highlight(m_hand.size(), false);
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            if (isValidAttack(table, m_hand.card(i)))
            {
                highlight[i] = true;
            }
        }
        showHandForChoose(highlight);

        int cardIndex = -1;
        while (cardIndex < 0 || cardIndex > m_hand.size()) {
            std::cout << "Выберите карту для подкидывания (1 - " << m_hand.size() << ", 0 - пропустить): ";
            std::cin >> cardIndex;

            if (cardIndex == 0) return Card();
            if (highlight[cardIndex - 1]) break;

            std::cout << "Эту карту нельзя выбрать, попробуй еще раз!\n";
            cardIndex = -1;
        }

        Card card = m_hand.take(cardIndex - 1);
        return card;
    }

private:
    bool isAI;
    std::string name;
    Hand m_hand;

    bool isValidAttack(const Table& table, const Card& card) const
    {
        for (size_t i = 0; i < table.size(); ++i)
        {
            if (table.card(i).rank == card.rank) return true;
        }
        return table.empty();
    }

    bool isValidDefense(const Card& attack, const Card& defend, const Card& trump) const 
    {
        if (defend.suit == attack.suit && defend.rank > attack.rank) return true;
        if (defend.suit == trump.suit  && attack.suit != trump.suit) return true;
        return false;
    }

    const Card chooseLowestWeightCard(const Cards& validCards, const Card& trumpCard)
    {
        auto compare = [&trumpCard](const Card& a, const Card& b) -> bool
        {
            int aWeight = a.rank; if (a.suit == trumpCard.suit) aWeight += RANK_ACE;
            int bWeight = b.rank; if (b.suit == trumpCard.suit) bWeight += RANK_ACE;
            return aWeight < bWeight;
        };
        return *std::min_element(validCards.begin(), validCards.end(), compare);
    }

    const Card aiChooseAttackCard(const Card& trumpCard) 
    {
        std::vector<Card> possibleAttacks;
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            possibleAttacks.push_back(m_hand.card(i));
        }
        return chooseLowestWeightCard(possibleAttacks, trumpCard);
    }

    const Card aiChooseDefenseCard(const Card& attackCard, const Card& trumpCard) 
    {
        std::vector<Card> possibleDefenses;
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            const Card& handCard = m_hand.card(i);
            if (isValidDefense(attackCard, handCard, trumpCard))
            {
                possibleDefenses.push_back(handCard);
            }
        }
        return (possibleDefenses.empty() ? NothingCard : chooseLowestWeightCard(possibleDefenses, trumpCard));
    }

    Card aiChooseAdditionalAttackCard(const Table& table, size_t defenderHandSize, const Card& trumpCard) 
    {
        if (defenderHandSize == 0)
            return NothingCard;

        std::vector<Card> possibleAttacks;
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            const Card& handCard = m_hand.card(i);
            if (isValidAttack(table, handCard))
            {
                possibleAttacks.push_back(handCard);
            }
        }
        return (possibleAttacks.empty() ? NothingCard : chooseLowestWeightCard(possibleAttacks, trumpCard));
    }

    bool hasValidAttacks(const Table& table) const 
    {
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            if (isValidAttack(table, m_hand.card(i))) return true;
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
                player.addCard(m_table.deck().take());

        // Определение первого атакующего игрока
        m_attackerIndex = findFirstAttacker(m_table.deck().trump());
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
            Card attackCard = attacker.attack(m_table.deck().trump());
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

            m_table.place(attackCard);
            std::cout << "Карты на столе:\n";
            m_table.print();

            bool defenderTookCards = false;
            while (true)
            {
                // Защитник пытается отбиться от последней подкинутой карты
                Card defendCard = defender.defense(m_table.last(), m_table.deck().trump());
                if (defendCard)
                {
                    std::cout << defender.getName() << " отбивается картой: ";
                    defendCard.print();
                    std::cout << "\n";
                    defender.showHandForInspect();

                    m_table.place(defendCard);
                    std::cout << "Карты на столе:\n";
                    m_table.print();
                }
                else
                {
                    std::cout << defender.getName() << " не может отбиться, забирает карты.\n";

                    defenderTookCards = true;
                    m_table.give(defender.getHand());
                    break;
                }

                // Проверяем, можно ли подкидывать еще карты
                if (!defender.hasCards()) break;

                // Подкидывание карт
                bool someoneAddedCard = false;
                for (size_t i = 0; i < m_players.size(); ++i)
                {
                    size_t attackerIndex = (m_attackerIndex + i) % m_players.size();
                    if (attackerIndex == defenderIndex) continue;
                    Player& attacker = m_players[attackerIndex];

                    Card additionalCard = attacker.additionalAttack(m_table, defender.getHand().size(), m_table.deck().trump());
                    if (additionalCard)
                    {
                        std::cout << "\n";
                        std::cout << attacker.getName() << " подкидывает карту: ";
                        additionalCard.print();
                        std::cout << "\n";
                        attacker.showHandForInspect();

                        m_table.place(additionalCard);
                        std::cout << "Карты на столе:\n";
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
                m_table.discard();
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
        std::cout << "Колода: ";
        m_table.deck().print();

        // Вывод карт игроков перед ходом
        std::cout << "Карты игроков перед ходом:\n";
        for (const auto& player : m_players)
        {
            player.showHandForInspect();
        }
        std::cout << "\n";
    }

    int findFirstAttacker(const Card& trumpCard) 
    {
        // Ищем игрока с наименьшей козырной картой
        int firstAttackerIndex = -1;
        Card lowestTrump{ RANK_ACE, trumpCard.suit };
        for (size_t i = 0; i < m_players.size(); ++i) 
        {
            const Hand& hand = m_players[i].getHand();
            for (size_t c = 0; c < hand.size(); ++c)
            {
                const Card& card = hand.card(c);
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
        std::vector<size_t> refillOrder;
        refillOrder.push_back(m_attackerIndex);
        for (size_t i = 1; i < m_players.size(); ++i)
            refillOrder.push_back((m_attackerIndex + i) % m_players.size());

        for (size_t idx : refillOrder)
        {
            Player& player = m_players[idx];
            while (player.getHand().size() < 6 && !m_table.deck().empty())
            {
                player.addCard(m_table.deck().take());
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
    Table m_table;
    std::vector<Player> m_players;
    int m_attackerIndex = 0;
    int m_roundNumber = 0;
};

int main() 
{
    SetConsoleOutputCP(CP_UTF8);
    Game game(!true);
    game.loop();
    return 0;
}
