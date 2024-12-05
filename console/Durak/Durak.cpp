#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <windows.h>
#include <string>
#include <sstream>
#include <set>

enum Suit { SUIT_NONE = -1, SUIT_HEARTS = 0, SUIT_DIAMONDS, SUIT_CLUBS, SUIT_SPADES };
enum Rank { RANK_NONE = -1, RANK_SIX = 6, RANK_SEVEN, RANK_EIGHT, RANK_NINE, RANK_TEN, RANK_JACK, RANK_QUEEN, RANK_KING, RANK_ACE };

struct Card 
{
    Rank rank = RANK_NONE;
    Suit suit = SUIT_NONE;

    operator bool() const { return (rank != RANK_NONE && suit != SUIT_NONE); }
    bool operator<  (const Card& other) const { return (rank == other.rank ? suit < other.suit : rank < other.rank); }

    void Print() const
    {
        const char* ranks[] = { "6", "7", "8", "9", "T", "J", "Q", "K", "A" };
        const char* suits[] = { "\u2665", "\u2666", "\u2663", "\u2660" }; // ♥ ♦ ♣ ♠
        std::cout << ranks[rank - RANK_SIX] << suits[suit];
    }
    void Println() const { Print(); std::cout << '\n'; }
};

using Cards = std::vector<Card>;
static const Card InvalidCard;

class Hand
{
public:
    using Cards = std::set<Card>;

    bool empty() const { return m_cards.empty(); }
    size_t size() const { return m_cards.size(); }

    void Give(const Card& card) { if (card) m_cards.insert(card); }
    void Take(const Card& card) { if (card) m_cards.erase(card); }

    const Card  TakeAt(size_t i) { Card card = CardAt(i); Take(card); return card; }
    const Card& CardAt(size_t i) const 
    {
        if (i < size())
        {
            auto it = m_cards.begin();
            std::advance(it, i);
            return *it;
        }
        return InvalidCard;
    }

    void Print() const
    {
        for (const auto& card : m_cards)
        {
            card.Print(); std::cout << ' ';
        }
    }

private:
    Cards m_cards;
};

class Deck
{
public:
    Deck()
    {
        Create(); Shuffle();
        m_trump = m_cards.front();
    }

    bool empty() const { return m_cards.empty(); }
    size_t size() const { return m_cards.size(); }
    const Card& Trump() const  { return m_trump; }

    const Card Take()
    {
        if (empty()) return InvalidCard;
        Card card = m_cards.back();
        m_cards.pop_back();
        return card;
    }

    void Refill(Hand& hand)
    {
        while (hand.size() < 6 && !empty()) hand.Give(Take());
    }

    void Print() const
    {
        m_trump.Print();
        std::cout << ' ' << m_cards.size() << '\n';
    }

private:
    void Create()
    {
        m_cards.resize(36);
        for (size_t i = 0; i < 36; ++i)
        {
            m_cards[i].rank = Rank(RANK_SIX + i / 4);
            m_cards[i].suit = Suit(i % 4);
        }
    }

    void Shuffle()
    {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(m_cards.begin(), m_cards.end(), g);
    }

private:
    Cards m_cards;
    Card  m_trump;
};

class Table
{
public:
    bool empty() const { return m_cards[0].empty(); }

    void PlaceAttack(const Card& card)  { if (card) m_cards[0].push_back(card); }
    void PlaceDefense(const Card& card) { if (card && m_cards[0].size() > m_cards[1].size()) m_cards[1].push_back(card); }

    const Card& LastAttack() const { return (m_cards[0].empty() ? InvalidCard : m_cards[0].back()); }
    const Card& Trump() const { return m_deck.Trump(); }

    Deck& GetDeck() { return m_deck; }
    const Cards& GetDiscard()  const { return m_discard;  }
    const Cards& GetAttacks()  const { return m_cards[0]; }
    const Cards& GetDefenses() const { return m_cards[1]; }

    void GiveTo(Hand& hand)
    {
        for (Cards& cards : m_cards)
        {
            for (const Card& card : cards) hand.Give(card);
            cards.clear();
        }
    }

    void Discard()
    {
        for (Cards& cards : m_cards)
        {
            for (const Card& card : cards) m_discard.push_back(card);
            cards.clear();
        }
    }

    void Println() const
    {
        for (size_t i = 0; i < m_cards[0].size(); ++i)
        {
            std::cout << std::string(5, ' ');
            m_cards[0][i].Print();
            if (i < m_cards[1].size())
            {
                std::cout << " X ";
                m_cards[1][i].Print();
            }
            std::cout << '\n';
        }
    }

private:
    Cards m_cards[2];
    Cards m_discard;
    Deck  m_deck;
};

class Player
{
public:
    Player(const std::string& name) : m_name(name) {}

    const std::string& name() const { return m_name; }
    Hand& hand() { return m_hand; }

    virtual const Card Attack (Table& table, Player& defender) = 0;
    virtual const Card Defense(Table& table, Player& attacker) = 0;
    virtual const Card Throwin(Table& table, Player& defender) = 0;

    void Print()   const { std::cout << m_name << ": "; m_hand.Print(); }
    void Println() const { Print(); std::cout << '\n'; }

protected:
    bool isValidAttack(const Table& table, const Card& card) const
    {
        for (const Card& tableCard : table.GetAttacks())
            if (tableCard.rank == card.rank) return true;
        for (const Card& tableCard : table.GetDefenses())
            if (tableCard.rank == card.rank) return true;
        return table.empty();
    }

    bool isValidDefense(const Card& attack, const Card& defend, const Card& trump) const 
    {
        if (defend.suit == attack.suit && defend.rank > attack.rank) return true;
        if (defend.suit == trump.suit  && attack.suit != trump.suit) return true;
        return false;
    }

    bool isThrowinAllowed(const Table& table, Player& defender) const
    {
        if (table.GetAttacks().size() < 6 && !defender.hand().empty())
        {
            for (size_t i = 0; i < m_hand.size(); ++i)
            {
                if (isValidAttack(table, m_hand.CardAt(i))) return true;
            }
        }
        return false;
    }

protected:
    std::string m_name;
    Hand m_hand;
};

class HumanPlayer : public Player
{
public:
    using Highlight = std::vector<bool>;
    HumanPlayer(const std::string& name) : Player(name) {}

    const Card Attack(Table& table, Player& defender) override
    {
        Highlight hl(m_hand.size(), true);
        print(hl);
        int index = chooseCardIndex(hl, "Выберите карту для атаки", "");
        std::cout << '\n';

        Card card = m_hand.TakeAt(index - 1);
        return card;
    }

    const Card Defense(Table& table, Player& attacker) override
    {
        const Card& attack = table.LastAttack();
        const Card& trump  = table.Trump();

        std::cout << '\n';
        Highlight hl(m_hand.size(), false);
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            if (isValidDefense(attack, m_hand.CardAt(i), trump)) hl[i] = true;
        }
        print(hl);
        int index = chooseCardIndex(hl, "Выберите карту для защиты", "взять карты");
        std::cout << '\n';
        
        Card card = m_hand.TakeAt(index - 1);
        return card;
    }

    const Card Throwin(Table& table, Player& defender) override
    {
        if (isThrowinAllowed(table, defender))
        {
            std::cout << '\n';
            Highlight hl(m_hand.size(), false);
            for (size_t i = 0; i < m_hand.size(); ++i)
            {
                if (isValidAttack(table, m_hand.CardAt(i))) hl[i] = true;
            }
            print(hl);
            int index = chooseCardIndex(hl, "Выберите карту для подкидывания", "пропустить");
            std::cout << '\n';
            
            Card card = m_hand.TakeAt(index - 1);
            return card;
        }
        return InvalidCard;
    }

private:
    int chooseCardIndex(const Highlight& hl, const std::string& msg, const std::string& act)
    {
        int index = -1, max = hl.size();
        while (index < 0 || index > max)
        {
            std::cout << msg << " (1 - " << max;
            if (!act.empty()) std::cout << ", 0 - " << act;
            std::cout << "): ";

            std::cin >> index;
            if (index == 0 && !act.empty()) break;
            if (index <= max && index > 0 && hl[index - 1]) break;

            std::cout << "Эту карту нельзя выбрать, попробуй еще раз!\n";
            index = -1;
        }
        return index;
    }

    void print(const Highlight& highlight) const
    {
        Player::Println();
        std::cout << m_name << ": ";
        for (size_t i = 0; i < highlight.size(); ++i)
        {
            std::cout << '^' << char(highlight[i] ? ('1' + i) : '.') << ' ';
        }
        std::cout << '\n';
    }
};

class AIPlayer : public Player
{
public:
    AIPlayer(const std::string& name) : Player(name) {}

    const Card Attack(Table& table, Player& defender) override
    {
        const Card card = chooseAttackCard(table, defender);
        m_hand.Take(card);
        return card;
    }

    const Card Defense(Table& table, Player& attacker) override
    {
        const Card card = chooseDefenseCard(table, attacker);
        m_hand.Take(card);
        return card;
    }

    const Card Throwin(Table& table, Player& defender) override
    {
        if (isThrowinAllowed(table, defender))
        {
            const Card card = chooseThrowinCard(table, defender);
            m_hand.Take(card);
            return card;
        }
        return InvalidCard;
    }

protected:
    virtual const Card chooseAttackCard(Table& table, Player& defender)
    {
        std::vector<Card> possibleAttacks;
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            possibleAttacks.push_back(m_hand.CardAt(i));
        }

        const Card& trump = table.Trump();
        return chooseLowestWeightCard(possibleAttacks, trump);
    }

    virtual const Card chooseDefenseCard(Table& table, Player& attacker)
    {
        const Card& attack = table.LastAttack();
        const Card& trump  = table.Trump();

        std::vector<Card> possibleDefenses;
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            const Card& hand = m_hand.CardAt(i);
            if (isValidDefense(attack, hand, trump))
            {
                possibleDefenses.push_back(hand);
            }
        }
        return (possibleDefenses.empty() ? InvalidCard : chooseLowestWeightCard(possibleDefenses, trump));
    }

    virtual const Card chooseThrowinCard(Table& table, Player& defender)
    {
        std::vector<Card> possibleAttacks;
        for (size_t i = 0; i < m_hand.size(); ++i)
        {
            const Card& hand = m_hand.CardAt(i);
            if (isValidAttack(table, hand))
            {
                possibleAttacks.push_back(hand);
            }
        }

        const Card& trump = table.Trump();
        return (possibleAttacks.empty() ? InvalidCard : chooseLowestWeightCard(possibleAttacks, trump));
    }

private:
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
};

class Game 
{
public:
    Game(bool useAIasHuman)
    {
        int numPlayers = 0;
        while (numPlayers < 2 || numPlayers > 4)
        {
            std::cout << "Введите количество игроков (2-4): ";
            std::cin >> numPlayers;
        }

        if (useAIasHuman)
            m_players.push_back(std::make_shared<AIPlayer>("Hum"));
        else
            m_players.push_back(std::make_shared<HumanPlayer>("Hum"));
        for (int i = 1; i < numPlayers; ++i)
            m_players.push_back(std::make_shared<AIPlayer>("Ai" + std::to_string(i)));

        for (int i = 0; i < 6; ++i)
            for (auto& player : m_players)
                player->hand().Give(m_table.GetDeck().Take());

        m_attackerIndex = findFirstAttacker(m_table.Trump());
        std::cout << "Первым ходит игрок: " << m_players[m_attackerIndex]->name() << '\n';
    }

    void loop()
    {
        while (!isGameOver())
        {
            if (m_players.size() <= 1)
                break;

            // TODO: something wrong with index after players deletion
            size_t defenderIndex = (m_attackerIndex + 1) % m_players.size();
            Player& attacker = *m_players[m_attackerIndex];
            Player& defender = *m_players[defenderIndex];
            printNewRoundBegin(attacker, defender);
            
            Card attackCard = attacker.Attack(m_table, defender);
            m_table.PlaceAttack(attackCard);

            attacker.Print();
            std::cout << "-> ";
            attackCard.Println();
            m_table.Println();

            bool defenderTookCards = false;
            while (true)
            {
                Card defenseCard = defender.Defense(m_table, attacker);
                if (defenseCard)
                {
                    m_table.PlaceDefense(defenseCard);

                    defender.Print();
                    std::cout << "-> ";
                    defenseCard.Println();
                    m_table.Println();
                }
                else
                {
                    m_table.GiveTo(defender.hand());
                    std::cout << defender.name() << ": не может отбиться, забирает карты.\n";

                    defenderTookCards = true;
                    break;
                }

                bool someoneThrewCard = false;
                for (size_t i = 0; i < m_players.size(); ++i)
                {
                    size_t attackerIndex = (m_attackerIndex + i) % m_players.size();
                    if (attackerIndex == defenderIndex) continue;
                    Player& attacker = *m_players[attackerIndex];

                    Card throwinCard = attacker.Throwin(m_table, defender);
                    if (throwinCard)
                    {
                        m_table.PlaceAttack(throwinCard);

                        attacker.Print();
                        std::cout << "-> ";
                        throwinCard.Println();
                        m_table.Println();

                        someoneThrewCard = true;
                        break;
                    }
                }
                if (!someoneThrewCard) break;
            }
            if (!defenderTookCards)
            {
                m_table.Discard();
                std::cout << defender.name() << ": успешно отбился, карты идут в отбой.\n";
            }

            /////////////////////////////////////////////////////////////////////

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
    void printNewRoundBegin(const Player& attacker, const Player& defender)
    {
        std::cout << '\n' << std::string(5, '~');
        std::cout << " ROUND: " << ++m_roundNumber << ' ';
        std::cout << attacker.name() << " -> " << defender.name() << ' ';
        std::cout << std::string(5, '~') << '\n';

        std::cout << "\n";
        std::cout << "Колода: ";
        m_table.GetDeck().Print();

        std::cout << "Карты игроков:\n";
        for (const auto& player : m_players) player->Println();
        std::cout << "\n";
    }

    int findFirstAttacker(const Card& trumpCard) 
    {
        // Ищем игрока с наименьшей козырной картой
        int firstAttackerIndex = -1;
        Card lowestTrump{ RANK_ACE, trumpCard.suit };
        for (size_t i = 0; i < m_players.size(); ++i) 
        {
            const Hand& hand = m_players[i]->hand();
            for (size_t c = 0; c < hand.size(); ++c)
            {
                const Card& card = hand.CardAt(c);
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
            Player& player = *m_players[idx];
            m_table.GetDeck().Refill(player.hand());
        }
    }

    void removePlayersWithoutCards() {
        m_players.erase(remove_if(m_players.begin(), m_players.end(),
            [](std::shared_ptr<Player>& player)
            {
                return player->hand().empty();
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
            std::cout << m_players[0]->name() << ": проиграл и остается дураком!\n";
            m_players[0]->Print();
        }
    }

private:
    Table m_table;
    std::vector<std::shared_ptr<Player>> m_players;
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
