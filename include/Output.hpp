#pragma once

#include <unordered_map>
#include <map>
#include <deque>
#include <array>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>

using Position = std::array<unsigned int, 2u>;

namespace std
{
  template<>
  struct hash<Position>
  {
    constexpr std::size_t operator()(Position const &array) const noexcept
    {
      return array[0] | (static_cast<std::size_t>(array[1]) << 32ul);
    }
  };
}

class Display
{
public:
  constexpr static unsigned int size{12u};

  void printTile(Position const &pos, int ch) const noexcept
  {
    mvaddch(pos[1], pos[0] * 2, ch);
  }

  template<class... T>
  void printMsg(char const *fmt, T &&... more) const noexcept
  {
    mvprintw(size * 2 + 1, 0, fmt, std::forward<T>(more)...);
  }
};

inline std::optional<Position> getDir(int ch)
{
  if (ch == KEY_RIGHT)
    {
      return std::optional<Position>({1u, 0u});
    }
  if (ch == KEY_UP)
    {
      return std::optional<Position>({0u, -1u});
    }
  if (ch == KEY_LEFT)
    {
      return std::optional<Position>({-1u, 0u});
    }
  if (ch == KEY_DOWN)
    {
      return std::optional<Position>({0u, 1u});
    }
  return std::optional<Position>{};
}


constexpr int sign(int a) noexcept
{
  return (a > 0) - (a < 0);
}

struct Tile
{
  static constexpr unsigned int wall{-2u};
  static constexpr unsigned int empty{-1u};

  unsigned int entityIndex;

  bool isEmpty() const noexcept
  {
    return entityIndex == empty;
  }

  auto &getEntityIndex() noexcept
  {
    return entityIndex;
  }

  auto const &getEntityIndex() const noexcept
  {
    return entityIndex;
  }
};

struct Entity
{
  Position pos;
  int symbol;
  bool dead{false};

  auto getSymbol() const noexcept
  {
    return symbol;
  }

  Position const &getPos() const noexcept
  {
    return pos;
  }
};

class Logic : private Display
{
  std::unordered_map<Position, Tile> tiles;
  bool needsRefresh{true};
  std::deque<Entity> entities;
  std::default_random_engine rEngine{0u};
  std::default_random_engine rSpawnEngine{0u};
  static constexpr int WILL_WALK_TO_PLAYER{'+' | (COLOR_GREEN << 8)};
  static constexpr int WALK_TO_PLAYER{'+' | (COLOR_YELLOW << 8)};
  static constexpr int WALK_UP{'^' | (COLOR_YELLOW << 8)};
  static constexpr int WALK_DOWN{'V' | (COLOR_YELLOW << 8)};
  static constexpr int WALK_LEFT{'<' | (COLOR_YELLOW << 8)};
  static constexpr int WALK_RIGHT{'>' | (COLOR_YELLOW << 8)};
  static constexpr int WILL_SHOOT_AT_PLAYER{'H' | (COLOR_GREEN << 8)};
  static constexpr int SHOOT_AT_PLAYER{'H' | (COLOR_YELLOW << 8)};
  static constexpr int PLAYER{'@' | (COLOR_YELLOW << 8)};

  auto &getTileImpl(Position pos)
  {
    try {
      return tiles.at(pos);
    } catch (std::out_of_range const &){
      auto &tile(tiles[pos]);

      tile.getEntityIndex() = std::uniform_int_distribution<unsigned int>(0, 4)(rEngine) ? Tile::empty : Tile::wall;
      switch (std::uniform_int_distribution<unsigned int>(0, 420)(rEngine)) {
      case 5:
      	insertEntity(Entity{pos, SHOOT_AT_PLAYER});
	break;
      case 7:
      	insertEntity(Entity{pos, WALK_UP});
	break;
      case 8:
      	insertEntity(Entity{pos, WALK_DOWN});
	break;
      case 9:
      	insertEntity(Entity{pos, WALK_LEFT});
	break;
      case 10:
      	insertEntity(Entity{pos, WALK_RIGHT});
	break;
      case 11:
      	insertEntity(Entity{pos, WALK_TO_PLAYER});
	break;
      }
      return tiles.at(pos);
    }
  }
public:
  Logic()
  {
    insertEntity(Entity{{0, 0}, PLAYER}, 0u);
    for (unsigned int i(-(size / 2u)); i != size / 2u + 1; ++i)
      for (unsigned int j(-(size / 2u)); j != size / 2u + 1; ++j)
	if (i != 0 || j != 0)
	  killTile({i, j});
  }
  
  decltype(auto) getPlayerPosition() const noexcept
  {
    return entities[0].getPos();
  }

  static constexpr std::chrono::milliseconds displayDelay{10};

  auto getTile(Position pos)
  {
    return getTileImpl(pos);
  }

  void print()
  {
    if (needsRefresh)
      {
	std::this_thread::sleep_for(displayDelay);
        auto playerPos(getPlayerPosition());

	for (unsigned int i(0u); i <= size * 2u; ++i)
	  for (unsigned int j(0u); j <= size * 2u; ++j)
	    {
	      auto entityIndex(getTile({playerPos[0] + i - size, playerPos[1] + j - size}).getEntityIndex());
	      int ch;

	      if (entityIndex == Tile::empty)
		ch = '.';
	      else if (entityIndex == Tile::wall)
		ch = 'x' | (COLOR_BLUE << 8);
	      else
		ch = entities[entityIndex].getSymbol();
	      printTile({i, j}, ch);
	    }
      }
  }

  auto &getWritableTile(Position pos)
  {
    auto playerPos(getPlayerPosition());

    needsRefresh |= (pos[0] - playerPos[0]) <= size * 2 || (pos[1] - playerPos[1]) <= size * 2;
    return getTileImpl(pos);
  }

  void insertEntity(Entity const &entity, unsigned int key = 1u)
  {
    killTile(entity.pos);
  retry:
    if (key == entities.size())
      entities.push_back(entity);
    else if (entities[key].dead)
      entities[key] = entity;
    else
      {
	++key;
	goto retry;
      }
    getWritableTile(entity.pos).entityIndex = key;
  }

  bool moveTo(size_t index, Position dest)
  {
    Entity &entity(entities[index]);

    if (getTile(dest).isEmpty())
      {
	getWritableTile(entity.pos).getEntityIndex() = Tile::empty;
	entity.pos = dest;
	getWritableTile(entity.pos).getEntityIndex() = index;
	needsRefresh = true;
	return true;
      }
    return false;
  }

  void killTile(Position dest)
  {
    auto entityIndex(getTile(dest).getEntityIndex());

    if (entityIndex != Tile::wall && entityIndex != Tile::empty)
      {
	entities[entityIndex].dead = true;
      }
    getWritableTile(dest).getEntityIndex() = Tile::empty;
  }

  void moveOrDie(size_t index, Position dest)
  {
    Entity &entity(entities[index]);
    if (!moveTo(index, dest))
      {
	killTile(entity.pos);
	killTile(dest);
      }
  }

  void action(Position dir)
  {
    auto &player(entities.front());
    moveTo(0u, {player.pos[0] + dir[0], player.pos[1] + dir[1]});
  }

  bool nextInput()
  {
    constexpr char quit_key{'q'};
    constexpr char restart_key{'r'};
    int ch(getch());

    if (ch == restart_key)
      {
	*this = Logic();
	print();
	return true;
      }
    if (ch == quit_key)
      return false;
    if (auto optional = getDir(ch))
      action(*optional);
    else
      printMsg("Unknown key '%c'. Press '%c' to quit.", ch, quit_key);
    print();

    for (size_t i(1u); i < entities.size(); ++i)
      if (!entities[i].dead)
	{
	  update(i);
	  print();
	}
    if (entities.front().dead) {
      printMsg("You died. Press '%c' to restart or '%c' to quit", restart_key, quit_key);
      while (true)
	{
	  int ch2(getch());

	  if (ch2 == restart_key)
	    {
	      *this = Logic();
	      print();
	      return true;
	    }
	  else if (ch2 == quit_key)
	    return false;
	  printMsg("Unknown key. You died. Press '%c' to restart or '%c' to quit", restart_key, quit_key);
	}
    }
    return true;
  }

  void update(size_t index)
  {
    Entity &entity(entities[index]);
    auto target(getPlayerPosition());

    auto getCloserAxis([](Position a, Position b)
		       {
			 return std::abs(static_cast<int>(a[0] - b[0])) > std::abs(static_cast<int>(a[1] - b[1]));
		       });
    switch (entity.getSymbol())
      {
      case PLAYER:
	return ;
      case WILL_WALK_TO_PLAYER:
	{
	  entity.symbol = WALK_TO_PLAYER;
	  return ;
	}
      case WALK_TO_PLAYER:
	{
	  entity.symbol = WILL_WALK_TO_PLAYER;
	  moveOrDie(index, {entity.pos[0] + sign(target[0] - entity.pos[0]),
		entity.pos[1] + sign(target[1] - entity.pos[1])});
	  return ;
	}
      case WALK_UP:
	moveOrDie(index, {entity.pos[0], entity.pos[1] - 1});
	return ;
      case WALK_DOWN:
	moveOrDie(index, {entity.pos[0], entity.pos[1] + 1});
	return ;
      case WALK_RIGHT:
	moveOrDie(index, {entity.pos[0] + 1, entity.pos[1]});
	return ;
      case WALK_LEFT:
	moveOrDie(index, {entity.pos[0] - 1, entity.pos[1]});
	return ;
      case WILL_SHOOT_AT_PLAYER:
	{
	  entity.symbol = SHOOT_AT_PLAYER;
	  
	  if (!moveTo(index, {entity.pos[0] + sign(target[0] - entity.pos[0]),
		  entity.pos[1] + sign(target[1] - entity.pos[1])}))
	    {
	      if (getCloserAxis(target, entity.pos))
		moveTo(index, {entity.pos[0] + sign(target[0] - entity.pos[0]),
		      entity.pos[1]});
	      else
		moveTo(index, {entity.pos[0],
		      entity.pos[1] + sign(target[1] - entity.pos[1])});
		  
	    }
	  return ;
	}
      case SHOOT_AT_PLAYER:
	{
	  entity.symbol = WILL_SHOOT_AT_PLAYER;
	  if (!getCloserAxis(target, entity.pos))
	    {
	      if (static_cast<int>(target[1] - entity.pos[1]) > 0)
		insertEntity(Entity{{entity.pos[0], entity.pos[1] + 1}, WALK_DOWN});
	      else
		insertEntity(Entity{{entity.pos[0], entity.pos[1] - 1}, WALK_UP});
	    }
	  else
	    {
	      if (static_cast<int>(target[0] - entity.pos[0]) > 0)
		insertEntity(Entity{{entity.pos[0] + 1, entity.pos[1]}, WALK_RIGHT});
	      else
		insertEntity(Entity{{entity.pos[0] - 1, entity.pos[1]}, WALK_LEFT});
	    }
	}
	return ;
      }
  }
  };
