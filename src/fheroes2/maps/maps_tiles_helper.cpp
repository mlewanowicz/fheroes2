/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "maps_tiles_helper.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <list>
#include <optional>
#include <ostream>
#include <type_traits>

#include "army_troop.h"
#include "artifact.h"
#include "color.h"
#include "direction.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "monster.h"
#include "mp2.h"
#include "pairs.h"
#include "payment.h"
#include "profit.h"
#include "rand.h"
#include "resource.h"
#include "skill.h"
#include "spell.h"
#include "tools.h"
#include "week.h"
#include "world.h"

namespace Maps
{
    int32_t getMineSpellIdFromTile( const Tiles & tile )
    {
        return tile.getAdditionalMetadata();
    }

    void setMineSpellOnTile( Tiles & tile, const int32_t spellId )
    {
        tile.setAdditionalMetadata( spellId );
    }

    Spell getSpellFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_ARTIFACT:
            if ( tile.QuantityVariant() == 15 ) {
                return { tile.GetQuantity1() };
            }
            return { Spell::NONE };

        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE:
        case MP2::OBJ_PYRAMID:
            return { tile.GetQuantity1() };

        default:
            break;
        }

        return { Spell::NONE };
    }

    void setSpellOnTile( Tiles & tile, const int spellId )
    {
        using Quantity1Type = decltype( tile.GetQuantity1() );
        static_assert( std::is_same_v<Quantity1Type, uint8_t>, "Type of GetQuantity1() has been changed, check the logic below" );

        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE:
        case MP2::OBJ_PYRAMID:
            assert( spellId >= std::numeric_limits<Quantity1Type>::min() && spellId <= std::numeric_limits<Quantity1Type>::max() );

            tile.setQuantity1( static_cast<Quantity1Type>( spellId ) );
            break;

        default:
            break;
        }
    }

    void setMonsterOnTileJoinCondition( Tiles & tile, const int32_t condition )
    {
        tile.setAdditionalMetadata( condition );
    }

    bool isMonsterOnTileJoinConditionSkip( const Tiles & tile )
    {
        return tile.GetObject() == MP2::OBJ_MONSTER && tile.getAdditionalMetadata() == Monster::JOIN_CONDITION_SKIP;
    }

    bool isMonsterOnTileJoinConditionFree( const Tiles & tile )
    {
        return tile.GetObject() == MP2::OBJ_MONSTER && tile.getAdditionalMetadata() == Monster::JOIN_CONDITION_FREE;
    }

    Monster getMonsterFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_WATCH_TOWER:
            return { Monster::ORC };
        case MP2::OBJ_EXCAVATION:
            return { Monster::SKELETON };
        case MP2::OBJ_CAVE:
            return { Monster::CENTAUR };
        case MP2::OBJ_TREE_HOUSE:
            return { Monster::SPRITE };
        case MP2::OBJ_ARCHER_HOUSE:
            return { Monster::ARCHER };
        case MP2::OBJ_GOBLIN_HUT:
            return { Monster::GOBLIN };
        case MP2::OBJ_DWARF_COTTAGE:
            return { Monster::DWARF };
        case MP2::OBJ_HALFLING_HOLE:
            return { Monster::HALFLING };
        case MP2::OBJ_PEASANT_HUT:
            return { Monster::PEASANT };
        case MP2::OBJ_RUINS:
            return { Monster::MEDUSA };
        case MP2::OBJ_TREE_CITY:
            return { Monster::SPRITE };
        case MP2::OBJ_WAGON_CAMP:
            return { Monster::ROGUE };
        case MP2::OBJ_DESERT_TENT:
            return { Monster::NOMAD };
        case MP2::OBJ_TROLL_BRIDGE:
            return { Monster::TROLL };
        case MP2::OBJ_DRAGON_CITY:
            return { Monster::RED_DRAGON };
        case MP2::OBJ_CITY_OF_DEAD:
            return { Monster::POWER_LICH };
        case MP2::OBJ_GENIE_LAMP:
            return { Monster::GENIE };
        case MP2::OBJ_ABANDONED_MINE:
            return { Monster::GHOST };
        // Price of Loyalty
        case MP2::OBJ_WATER_ALTAR:
            return { Monster::WATER_ELEMENT };
        case MP2::OBJ_AIR_ALTAR:
            return { Monster::AIR_ELEMENT };
        case MP2::OBJ_FIRE_ALTAR:
            return { Monster::FIRE_ELEMENT };
        case MP2::OBJ_EARTH_ALTAR:
            return { Monster::EARTH_ELEMENT };
        case MP2::OBJ_BARROW_MOUNDS:
            return { Monster::GHOST };

        case MP2::OBJ_MONSTER:
            return { tile.GetObjectSpriteIndex() + 1 };
        default:
            break;
        }

        if ( MP2::isCaptureObject( tile.GetObject( false ) ) ) {
            return { world.GetCapturedObject( tile.GetIndex() ).GetTroop().GetID() };
        }

        return { Monster::UNKNOWN };
    }

    Artifact getArtifactFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_WAGON:
            return { tile.GetQuantity2() ? static_cast<int>( Artifact::UNKNOWN ) : tile.GetQuantity1() };

        case MP2::OBJ_SKELETON:
        case MP2::OBJ_DAEMON_CAVE:
        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_TREASURE_CHEST:
        case MP2::OBJ_SHIPWRECK_SURVIVOR:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
            return { tile.GetQuantity1() };

        case MP2::OBJ_ARTIFACT:
            if ( tile.QuantityVariant() == 15 ) {
                Artifact art( Artifact::SPELL_SCROLL );
                art.SetSpell( getSpellFromTile( tile ).GetID() );
                return art;
            }
            else
                return { tile.GetQuantity1() };

        default:
            break;
        }

        return { Artifact::UNKNOWN };
    }

    void setArtifactOnTile( Tiles & tile, const int artifactId )
    {
        using Quantity1Type = decltype( tile.GetQuantity1() );
        static_assert( std::is_same_v<Quantity1Type, uint8_t>, "Type of GetQuantity1() has been changed, check the logic below" );

        assert( artifactId >= std::numeric_limits<Quantity1Type>::min() && artifactId <= std::numeric_limits<Quantity1Type>::max() );

        tile.setQuantity1( static_cast<Quantity1Type>( artifactId ) );
    }

    uint32_t getGoldAmountFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_ARTIFACT:
            switch ( tile.QuantityVariant() ) {
            case 1:
                return 2000;
            case 2:
                return 2500;
            case 3:
                return 3000;
            default:
                break;
            }
            break;

        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_MAGIC_GARDEN:
        case MP2::OBJ_WATER_WHEEL:
        case MP2::OBJ_TREE_OF_KNOWLEDGE:
            return tile.GetQuantity1() == Resource::GOLD ? 100 * tile.GetQuantity2() : 0;

        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_TREASURE_CHEST:
        case MP2::OBJ_DERELICT_SHIP:
        case MP2::OBJ_GRAVEYARD:
            return 100 * tile.GetQuantity2();

        case MP2::OBJ_DAEMON_CAVE:
            switch ( tile.QuantityVariant() ) {
            case 2:
            case 4:
                return 2500;
            default:
                break;
            }
            break;

        case MP2::OBJ_SHIPWRECK:
            switch ( tile.QuantityVariant() ) {
            case 1:
                return 1000;
            case 2:
            case 4:
                // Case 4 gives 2000 gold and an artifact.
                return 2000;
            case 3:
                return 5000;
            default:
                break;
            }
            break;

        default:
            break;
        }

        return 0;
    }

    Skill::Secondary getSecondarySkillFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_ARTIFACT:
            switch ( tile.QuantityVariant() ) {
            case 4:
                return { Skill::Secondary::LEADERSHIP, Skill::Level::BASIC };
            case 5:
                return { Skill::Secondary::WISDOM, Skill::Level::BASIC };
            default:
                break;
            }
            break;

        case MP2::OBJ_WITCHS_HUT:
            return { tile.GetQuantity1(), Skill::Level::BASIC };

        default:
            break;
        }

        return {};
    }

    void setSecondarySkillOnTile( Tiles & tile, const int skillId )
    {
        using Quantity1Type = decltype( tile.GetQuantity1() );
        static_assert( std::is_same_v<Quantity1Type, uint8_t>, "Type of GetQuantity1() has been changed, check the logic below" );

        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_WITCHS_HUT:
            assert( skillId >= std::numeric_limits<Quantity1Type>::min() && skillId <= std::numeric_limits<Quantity1Type>::max() );

            tile.setQuantity1( static_cast<Quantity1Type>( skillId ) );
            break;

        default:
            break;
        }
    }

    ResourceCount getResourcesFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_ARTIFACT:
            switch ( tile.QuantityVariant() ) {
            case 1:
                return { Resource::GOLD, getGoldAmountFromTile( tile ) };
            case 2:
                return { Resource::getResourceTypeFromIconIndex( tile.QuantityExt() - 1 ), 3 };
            case 3:
                return { Resource::getResourceTypeFromIconIndex( tile.QuantityExt() - 1 ), 5 };
            default:
                break;
            }
            break;

        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_TREASURE_CHEST:
            return { Resource::GOLD, getGoldAmountFromTile( tile ) };

        case MP2::OBJ_FLOTSAM:
            return { Resource::WOOD, tile.GetQuantity1() };

        default:
            break;
        }

        return { tile.GetQuantity1(), Resource::GOLD == tile.GetQuantity1() ? getGoldAmountFromTile( tile ) : tile.GetQuantity2() };
    }

    void setResourceOnTile( Tiles & tile, const int resourceType, uint32_t value )
    {
        using Quantity1Type = decltype( tile.GetQuantity1() );
        using Quantity2Type = decltype( tile.GetQuantity2() );
        static_assert( std::is_same_v<Quantity1Type, uint8_t> && std::is_same_v<Quantity2Type, uint8_t>,
                       "Types of tile's quantities have been changed, check the logic below" );

        assert( resourceType >= std::numeric_limits<Quantity1Type>::min() && resourceType <= std::numeric_limits<Quantity1Type>::max() );

        tile.setQuantity1( static_cast<Quantity1Type>( resourceType ) );

        if ( resourceType == Resource::GOLD ) {
            value = value / 100;
        }

        assert( value >= std::numeric_limits<Quantity2Type>::min() && value <= std::numeric_limits<Quantity2Type>::max() );

        tile.setQuantity2( static_cast<Quantity2Type>( value ) );
    }

    Funds getFundsFromTile( const Tiles & tile )
    {
        const ResourceCount & rc = getResourcesFromTile( tile );

        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_ARTIFACT:
            switch ( tile.QuantityVariant() ) {
            case 1:
                return Funds( rc );
            case 2:
            case 3:
                return Funds( Resource::GOLD, getGoldAmountFromTile( tile ) ) + Funds( rc );
            default:
                break;
            }
            break;

        case MP2::OBJ_CAMPFIRE:
            return Funds( Resource::GOLD, getGoldAmountFromTile( tile ) ) + Funds( rc );

        case MP2::OBJ_FLOTSAM:
            return Funds( Resource::GOLD, getGoldAmountFromTile( tile ) ) + Funds( Resource::WOOD, tile.GetQuantity1() );

        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_TREASURE_CHEST:
        case MP2::OBJ_DERELICT_SHIP:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_DAEMON_CAVE:
            return { Resource::GOLD, getGoldAmountFromTile( tile ) };

        default:
            break;
        }

        return Funds( rc );
    }

    Troop getTroopFromTile( const Tiles & tile )
    {
        return MP2::isCaptureObject( tile.GetObject( false ) ) ? world.GetCapturedObject( tile.GetIndex() ).GetTroop()
                                                               : Troop( getMonsterFromTile( tile ), getMonsterCountFromTile( tile ) );
    }

    int getColorFromTile( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_BARRIER:
        case MP2::OBJ_TRAVELLER_TENT:
            return tile.GetQuantity1();

        default:
            return world.ColorCapturedObject( tile.GetIndex() );
        }
    }

    void setColorOnTile( Tiles & tile, const int color )
    {
        using Quantity1Type = decltype( tile.GetQuantity1() );
        static_assert( std::is_same_v<Quantity1Type, uint8_t>, "Type of GetQuantity1() has been changed, check the logic below" );

        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_BARRIER:
        case MP2::OBJ_TRAVELLER_TENT:
            assert( color >= std::numeric_limits<Quantity1Type>::min() && color <= std::numeric_limits<Quantity1Type>::max() );

            tile.setQuantity1( static_cast<Quantity1Type>( color ) );
            break;

        default:
            world.CaptureObject( tile.GetIndex(), color );
            break;
        }
    }

    bool doesTileContainValuableItems( const Tiles & tile )
    {
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_SHIPWRECK_SURVIVOR:
        case MP2::OBJ_TREASURE_CHEST:
        case MP2::OBJ_SEA_CHEST:
            return true;

        case MP2::OBJ_PYRAMID:
            return getSpellFromTile( tile ).isValid();

        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_DERELICT_SHIP:
        case MP2::OBJ_WATER_WHEEL:
        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_LEAN_TO:
        case MP2::OBJ_MAGIC_GARDEN:
            return tile.GetQuantity2() != 0;

        case MP2::OBJ_SKELETON:
            return getArtifactFromTile( tile ) != Artifact::UNKNOWN;

        case MP2::OBJ_WAGON:
            return getArtifactFromTile( tile ) != Artifact::UNKNOWN || tile.GetQuantity2() != 0;

        case MP2::OBJ_DAEMON_CAVE:
            return tile.QuantityVariant() != 0;

        default:
            break;
        }

        return false;
    }

    void resetObjectInfoOnTile( Tiles & tile )
    {
        // TODO: don't modify first 2 bits of quantity1.
        tile.setQuantity1( 0 );
        tile.setQuantity2( 0 );

        const MP2::MapObjectType objectType = tile.GetObject( false );

        switch ( objectType ) {
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_DAEMON_CAVE:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_SHIPWRECK_SURVIVOR:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_TREASURE_CHEST:
        case MP2::OBJ_WAGON:
            setArtifactOnTile( tile, Artifact::UNKNOWN );
            break;

        default:
            break;
        }

        if ( MP2::isPickupObject( objectType ) ) {
            tile.setAsEmpty();
        }
    }

    uint32_t getMonsterCountFromTile( const Tiles & tile )
    {
        static_assert( std::is_same_v<decltype( tile.GetQuantity1() ), uint8_t> && std::is_same_v<decltype( tile.GetQuantity2() ), uint8_t>,
                       "Types of tile's quantities have been changed, check the logic below" );

        // TODO: avoid this hacky way of storing data.
        return ( static_cast<uint32_t>( tile.GetQuantity1() ) << 8 ) + tile.GetQuantity2();
    }

    void setMonsterCountOnTile( Tiles & tile, uint32_t count )
    {
        static_assert( std::is_same_v<decltype( tile.GetQuantity1() ), uint8_t> && std::is_same_v<decltype( tile.GetQuantity2() ), uint8_t>,
                       "Types of tile's quantities have been changed, check the logic below" );

        if ( count > UINT16_MAX ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "The number of monsters for tile " << tile.GetIndex() << " is " << count << ", which is more than " << UINT16_MAX )

            count = UINT16_MAX;
        }

        // TODO: avoid this hacky way of storing data.
        tile.setQuantity1( ( count >> 8 ) & 0xFF );
        tile.setQuantity2( count & 0xFF );
    }

    void updateMonsterPopulationOnTile( Tiles & tile )
    {
        const Troop & troop = getTroopFromTile( tile );
        const uint32_t troopCount = troop.GetCount();

        if ( troopCount == 0 ) {
            setMonsterCountOnTile( tile, troop.GetRNDSize() );
        }
        else {
            const uint32_t bonusUnit = ( Rand::Get( 1, 7 ) <= ( troopCount % 7 ) ) ? 1 : 0;
            setMonsterCountOnTile( tile, troopCount * 8 / 7 + bonusUnit );
        }
    }

    void updateDwellingPopulationOnTile( Tiles & tile, bool isFirstLoad )
    {
        uint32_t count = isFirstLoad ? 0 : getMonsterCountFromTile( tile );
        const MP2::MapObjectType objectType = tile.GetObject( false );

        switch ( objectType ) {
        // join monsters
        case MP2::OBJ_HALFLING_HOLE:
            count += isFirstLoad ? Rand::Get( 20, 40 ) : Rand::Get( 5, 10 );
            break;
        case MP2::OBJ_PEASANT_HUT:
            count += isFirstLoad ? Rand::Get( 20, 50 ) : Rand::Get( 5, 10 );
            break;
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_TREE_HOUSE:
            count += isFirstLoad ? Rand::Get( 10, 25 ) : Rand::Get( 4, 8 );
            break;
        case MP2::OBJ_CAVE:
            count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 3, 6 );
            break;
        case MP2::OBJ_GOBLIN_HUT:
            count += isFirstLoad ? Rand::Get( 15, 40 ) : Rand::Get( 3, 6 );
            break;

        case MP2::OBJ_TREE_CITY:
            count += isFirstLoad ? Rand::Get( 20, 40 ) : Rand::Get( 10, 20 );
            break;

        case MP2::OBJ_WATCH_TOWER:
            count += isFirstLoad ? Rand::Get( 7, 10 ) : Rand::Get( 2, 4 );
            break;
        case MP2::OBJ_ARCHER_HOUSE:
            count += isFirstLoad ? Rand::Get( 10, 25 ) : Rand::Get( 2, 4 );
            break;
        case MP2::OBJ_DWARF_COTTAGE:
            count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 3, 6 );
            break;
        case MP2::OBJ_WAGON_CAMP:
            count += isFirstLoad ? Rand::Get( 30, 50 ) : Rand::Get( 3, 6 );
            break;
        case MP2::OBJ_DESERT_TENT:
            count += isFirstLoad ? Rand::Get( 10, 20 ) : Rand::Get( 1, 3 );
            break;
        case MP2::OBJ_RUINS:
            count += isFirstLoad ? Rand::Get( 3, 5 ) : Rand::Get( 1, 3 );
            break;
        case MP2::OBJ_WATER_ALTAR:
        case MP2::OBJ_AIR_ALTAR:
        case MP2::OBJ_FIRE_ALTAR:
        case MP2::OBJ_EARTH_ALTAR:
        case MP2::OBJ_BARROW_MOUNDS:
            count += Rand::Get( 2, 5 );
            break;

        case MP2::OBJ_TROLL_BRIDGE:
        case MP2::OBJ_CITY_OF_DEAD:
            count = isFirstLoad ? Rand::Get( 4, 6 ) : ( Color::NONE == getColorFromTile( tile ) ) ? count : count + Rand::Get( 1, 3 );
            break;

        case MP2::OBJ_DRAGON_CITY:
            count = isFirstLoad ? 2 : ( Color::NONE == getColorFromTile( tile ) ) ? count : count + 1;
            break;

        default:
            break;
        }

        if ( count ) {
            setMonsterCountOnTile( tile, count );
        }
    }

    void updateObjectInfoTile( Tiles & tile, const bool isFirstLoad )
    {
        // TODO: don't modify first 2 bits of quantity1.
        switch ( tile.GetObject( false ) ) {
        case MP2::OBJ_WITCHS_HUT:
            setSecondarySkillOnTile( tile, Skill::Secondary::RandForWitchsHut() );
            break;

        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
            setSpellOnTile( tile, Rand::Get( 1 ) ? Spell::RandCombat( 1 ).GetID() : Spell::RandAdventure( 1 ).GetID() );
            break;

        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
            setSpellOnTile( tile, Rand::Get( 1 ) ? Spell::RandCombat( 2 ).GetID() : Spell::RandAdventure( 2 ).GetID() );
            break;

        case MP2::OBJ_SHRINE_THIRD_CIRCLE:
            setSpellOnTile( tile, Rand::Get( 1 ) ? Spell::RandCombat( 3 ).GetID() : Spell::RandAdventure( 3 ).GetID() );
            break;

        case MP2::OBJ_SKELETON: {
            Rand::Queue percents( 2 );
            // 80%: empty
            percents.Push( 0, 80 );
            // 20%: artifact 1 or 2 or 3
            percents.Push( 1, 20 );

            if ( percents.Get() )
                setArtifactOnTile( tile, Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL ) );
            else
                resetObjectInfoOnTile( tile );
            break;
        }

        case MP2::OBJ_WAGON: {
            tile.setQuantity2( 0 );

            Rand::Queue percents( 3 );
            // 20%: empty
            percents.Push( 0, 20 );
            // 10%: artifact 1 or 2
            percents.Push( 1, 10 );
            // 50%: resource
            percents.Push( 2, 50 );

            switch ( percents.Get() ) {
            case 1:
                setArtifactOnTile( tile, Artifact::Rand( Rand::Get( 1 ) ? Artifact::ART_LEVEL_TREASURE : Artifact::ART_LEVEL_MINOR ) );
                break;
            case 2:
                setResourceOnTile( tile, Resource::Rand( false ), Rand::Get( 2, 5 ) );
                break;
            default:
                resetObjectInfoOnTile( tile );
                break;
            }
            break;
        }

        case MP2::OBJ_ARTIFACT: {
            const int art = Artifact::FromMP2IndexSprite( tile.GetObjectSpriteIndex() ).GetID();

            if ( Artifact::UNKNOWN != art ) {
                if ( art == Artifact::SPELL_SCROLL ) {
                    static_assert( std::is_same_v<decltype( tile.GetQuantity1() ), uint8_t> && std::is_same_v<decltype( tile.GetQuantity2() ), uint8_t>,
                                   "Types of tile's quantities have been changed, check the bitwise arithmetic below" );
                    static_assert( Spell::FIREBALL < Spell::SETWGUARDIAN, "The order of spell IDs has been changed, check the logic below" );

                    // Spell id of a spell scroll is represented by 2 low-order bits of quantity2 and 5 high-order bits of quantity1 plus one, and cannot be random
                    const int spell = std::clamp( ( ( tile.GetQuantity2() & 0x03 ) << 5 ) + ( tile.GetQuantity1() >> 3 ) + 1, static_cast<int>( Spell::FIREBALL ),
                                                  static_cast<int>( Spell::SETWGUARDIAN ) );

                    tile.QuantitySetVariant( 15 );
                    setSpellOnTile( tile, spell );
                }
                else {
                    // 0: 70% none
                    // 1,2,3 - 2000g, 2500g+3res, 3000g+5res,
                    // 4,5 - need to have skill wisdom or leadership,
                    // 6 - 50 rogues, 7 - 1 gin, 8,9,10,11,12,13 - 1 monster level4,
                    // 15 - spell
                    const int cond = Rand::Get( 1, 10 ) < 4 ? Rand::Get( 1, 13 ) : 0;

                    tile.QuantitySetVariant( cond );
                    setArtifactOnTile( tile, art );

                    if ( cond == 2 || cond == 3 ) {
                        // TODO: why do we use icon ICN index instead of map ICN index?
                        tile.QuantitySetExt( Resource::getIconIcnIndex( Resource::Rand( false ) ) + 1 );
                    }
                }
            }
            break;
        }

        case MP2::OBJ_RESOURCE: {
            int resourceType = Resource::UNKNOWN;

            if ( tile.getObjectIcnType() == MP2::OBJ_ICN_TYPE_OBJNRSRC ) {
                // The resource is located at the top.
                resourceType = Resource::FromIndexSprite( tile.GetObjectSpriteIndex() );
            }
            else {
                for ( TilesAddon & addon : tile.getLevel1Addons() ) {
                    if ( addon._objectIcnType == MP2::OBJ_ICN_TYPE_OBJNRSRC ) {
                        resourceType = Resource::FromIndexSprite( addon._imageIndex );
                        // If this happens we are in trouble. It looks like that map maker put the resource under an object which is impossible to do.
                        // Let's swap the addon and main tile objects
                        tile.swap( addon );

                        break;
                    }
                }
            }
            uint32_t count = 0;

            switch ( resourceType ) {
            case Resource::GOLD:
                count = 100 * Rand::Get( 5, 10 );
                break;
            case Resource::WOOD:
            case Resource::ORE:
                count = Rand::Get( 5, 10 );
                break;
            case Resource::MERCURY:
            case Resource::SULFUR:
            case Resource::CRYSTAL:
            case Resource::GEMS:
                count = Rand::Get( 3, 6 );
                break;
            default:
                // Some maps have broken resources being put which ideally we need to correct. Let's make them 0 Wood.
                DEBUG_LOG( DBG_GAME, DBG_WARN,
                           "Tile " << tile.GetIndex() << " contains unknown resource type. Object ICN type " << tile.getObjectIcnType() << ", image index "
                                   << tile.GetObjectSpriteIndex() )
                resourceType = Resource::WOOD;
                count = 0;
                break;
            }

            setResourceOnTile( tile, resourceType, count );
            break;
        }

        case MP2::OBJ_CAMPFIRE:
            // 4-6 rnd resource and + 400-600 gold
            setResourceOnTile( tile, Resource::Rand( false ), Rand::Get( 4, 6 ) );
            break;

        case MP2::OBJ_MAGIC_GARDEN:
            // 5 gems or 500 gold
            if ( Rand::Get( 1 ) )
                setResourceOnTile( tile, Resource::GEMS, 5 );
            else
                setResourceOnTile( tile, Resource::GOLD, 500 );
            break;

        case MP2::OBJ_WATER_WHEEL:
            // first week 500 gold, next week 1000 gold
            setResourceOnTile( tile, Resource::GOLD, ( 0 == world.CountDay() ? 500 : 1000 ) );
            break;

        case MP2::OBJ_WINDMILL: {
            int res = Resource::WOOD;
            while ( res == Resource::WOOD ) {
                res = Resource::Rand( false );
            }

            // 2 rnd resource
            setResourceOnTile( tile, res, 2 );
            break;
        }

        case MP2::OBJ_LEAN_TO:
            // 1-4 rnd resource
            setResourceOnTile( tile, Resource::Rand( false ), Rand::Get( 1, 4 ) );
            break;

        case MP2::OBJ_FLOTSAM: {
            switch ( Rand::Get( 1, 4 ) ) {
            // 25%: empty
            default:
                break;
            // 25%: 500 gold + 10 wood
            case 1:
                setResourceOnTile( tile, Resource::GOLD, 500 );
                tile.setQuantity1( 10 );
                break;
            // 25%: 200 gold + 5 wood
            case 2:
                setResourceOnTile( tile, Resource::GOLD, 200 );
                tile.setQuantity1( 5 );
                break;
            // 25%: 5 wood
            case 3:
                tile.setQuantity1( 5 );
                break;
            }
            break;
        }

        case MP2::OBJ_SHIPWRECK_SURVIVOR: {
            Rand::Queue percents( 3 );
            // 55%: artifact 1
            percents.Push( 1, 55 );
            // 30%: artifact 2
            percents.Push( 1, 30 );
            // 15%: artifact 3
            percents.Push( 1, 15 );

            // variant
            switch ( percents.Get() ) {
            case 1:
                setArtifactOnTile( tile, Artifact::Rand( Artifact::ART_LEVEL_TREASURE ) );
                break;
            case 2:
                setArtifactOnTile( tile, Artifact::Rand( Artifact::ART_LEVEL_MINOR ) );
                break;
            default:
                setArtifactOnTile( tile, Artifact::Rand( Artifact::ART_LEVEL_MAJOR ) );
                break;
            }
            break;
        }

        case MP2::OBJ_SEA_CHEST: {
            Rand::Queue percents( 3 );
            // 20% - empty
            percents.Push( 0, 20 );
            // 70% - 1500 gold
            percents.Push( 1, 70 );
            // 10% - 1000 gold + art
            percents.Push( 2, 10 );

            int art = Artifact::UNKNOWN;
            uint32_t gold = 0;

            // variant
            switch ( percents.Get() ) {
            default:
                break; // empty
            case 1:
                gold = 1500;
                break;
            case 2:
                gold = 1000;
                art = Artifact::Rand( Artifact::ART_LEVEL_TREASURE );
                break;
            }

            setResourceOnTile( tile, Resource::GOLD, gold );
            setArtifactOnTile( tile, art );
            break;
        }

        case MP2::OBJ_TREASURE_CHEST:
            if ( tile.isWater() ) {
                tile.SetObject( MP2::OBJ_SEA_CHEST );
                updateObjectInfoTile( tile, isFirstLoad );
                return;
            }

            {
                Rand::Queue percents( 4 );
                // 31% - 2000 gold or 1500 exp
                percents.Push( 1, 31 );
                // 32% - 1500 gold or 1000 exp
                percents.Push( 2, 32 );
                // 32% - 1000 gold or 500 exp
                percents.Push( 3, 32 );
                // 5% - art
                percents.Push( 4, 5 );

                int art = Artifact::UNKNOWN;
                uint32_t gold = 0;

                // variant
                switch ( percents.Get() ) {
                case 1:
                    gold = 2000;
                    break;
                case 2:
                    gold = 1500;
                    break;
                case 3:
                    gold = 1000;
                    break;
                default:
                    art = Artifact::Rand( Artifact::ART_LEVEL_TREASURE );
                    break;
                }

                setResourceOnTile( tile, Resource::GOLD, gold );
                setArtifactOnTile( tile, art );
            }
            break;

        case MP2::OBJ_DERELICT_SHIP:
            setResourceOnTile( tile, Resource::GOLD, 5000 );
            break;

        case MP2::OBJ_SHIPWRECK: {
            Rand::Queue percents( 4 );
            // 40% - 10ghost(1000g)
            percents.Push( 1, 40 );
            // 30% - 15 ghost(2000g)
            percents.Push( 2, 30 );
            // 20% - 25ghost(5000g)
            percents.Push( 3, 20 );
            // 10% - 50ghost(2000g+art)
            percents.Push( 4, 10 );

            const int cond = percents.Get();

            tile.QuantitySetVariant( cond );
            setArtifactOnTile( tile, cond == 4 ? Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL ) : Artifact::UNKNOWN );
            break;
        }

        case MP2::OBJ_GRAVEYARD:
            // 1000 gold + art
            setResourceOnTile( tile, Resource::GOLD, 1000 );
            setArtifactOnTile( tile, Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL ) );
            break;

        case MP2::OBJ_PYRAMID: {
            // random spell level 5
            const Spell & spell = Rand::Get( 1 ) ? Spell::RandCombat( 5 ) : Spell::RandAdventure( 5 );
            setSpellOnTile( tile, spell.GetID() );
            break;
        }

        case MP2::OBJ_DAEMON_CAVE: {
            // 1000 exp or 1000 exp + 2500 gold or 1000 exp + art or (-2500 or remove hero)
            const int cond = Rand::Get( 1, 4 );
            tile.QuantitySetVariant( cond );
            setArtifactOnTile( tile, cond == 3 ? Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL ) : Artifact::UNKNOWN );
            break;
        }

        case MP2::OBJ_TREE_OF_KNOWLEDGE:
            // variant: 10 gems, 2000 gold or free
            switch ( Rand::Get( 1, 3 ) ) {
            case 1:
                setResourceOnTile( tile, Resource::GEMS, 10 );
                break;
            case 2:
                setResourceOnTile( tile, Resource::GOLD, 2000 );
                break;
            default:
                break;
            }
            break;

        case MP2::OBJ_BARRIER:
            setColorOnTile( tile, getColorFromBarrierSprite( tile.getObjectIcnType(), tile.GetObjectSpriteIndex() ) );
            break;

        case MP2::OBJ_TRAVELLER_TENT:
            setColorOnTile( tile, getColorFromTravellerTentSprite( tile.getObjectIcnType(), tile.GetObjectSpriteIndex() ) );
            break;

        case MP2::OBJ_ALCHEMIST_LAB: {
            const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::MERCURY ).mercury );
            assert( resourceCount.has_value() && resourceCount > 0U );

            setResourceOnTile( tile, Resource::MERCURY, resourceCount.value() );
            break;
        }

        case MP2::OBJ_SAWMILL: {
            const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::WOOD ).wood );
            assert( resourceCount.has_value() && resourceCount > 0U );

            setResourceOnTile( tile, Resource::WOOD, resourceCount.value() );
            break;
        }

        case MP2::OBJ_MINES: {
            switch ( tile.GetObjectSpriteIndex() ) {
            case 0: {
                const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::ORE ).ore );
                assert( resourceCount.has_value() && resourceCount > 0U );

                setResourceOnTile( tile, Resource::ORE, resourceCount.value() );
                break;
            }
            case 1: {
                const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::SULFUR ).sulfur );
                assert( resourceCount.has_value() && resourceCount > 0U );

                setResourceOnTile( tile, Resource::SULFUR, resourceCount.value() );
                break;
            }
            case 2: {
                const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::CRYSTAL ).crystal );
                assert( resourceCount.has_value() && resourceCount > 0U );

                setResourceOnTile( tile, Resource::CRYSTAL, resourceCount.value() );
                break;
            }
            case 3: {
                const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::GEMS ).gems );
                assert( resourceCount.has_value() && resourceCount > 0U );

                setResourceOnTile( tile, Resource::GEMS, resourceCount.value() );
                break;
            }
            case 4: {
                const auto resourceCount = fheroes2::checkedCast<uint32_t>( ProfitConditions::FromMine( Resource::GOLD ).gold );
                assert( resourceCount.has_value() && resourceCount > 0U );

                setResourceOnTile( tile, Resource::GOLD, resourceCount.value() );
                break;
            }
            default:
                break;
            }
            break;
        }

        case MP2::OBJ_ABANDONED_MINE:
            // The number of Ghosts is set when loading the map and does not change anymore
            if ( isFirstLoad ) {
                setMonsterCountOnTile( tile, Rand::Get( 30, 60 ) );
            }
            break;

        case MP2::OBJ_BOAT:
            // This is a special case. Boats are different in the original editor.
            tile.setObjectIcnType( MP2::OBJ_ICN_TYPE_BOAT32 );
            tile.setObjectSpriteIndex( 18 );
            break;

        case MP2::OBJ_EVENT:
            tile.resetObjectSprite();
            break;

        case MP2::OBJ_RANDOM_ARTIFACT:
        case MP2::OBJ_RANDOM_ARTIFACT_TREASURE:
        case MP2::OBJ_RANDOM_ARTIFACT_MINOR:
        case MP2::OBJ_RANDOM_ARTIFACT_MAJOR:
            assert( isFirstLoad );

            updateRandomArtifact( tile );
            updateObjectInfoTile( tile, isFirstLoad );
            return;

        case MP2::OBJ_RANDOM_RESOURCE:
            assert( isFirstLoad );

            updateRandomResource( tile );
            updateObjectInfoTile( tile, isFirstLoad );
            return;

        case MP2::OBJ_MONSTER:
            if ( world.CountWeek() > 1 )
                updateMonsterPopulationOnTile( tile );
            else
                updateMonsterInfoOnTile( tile );
            break;

        case MP2::OBJ_RANDOM_MONSTER:
        case MP2::OBJ_RANDOM_MONSTER_WEAK:
        case MP2::OBJ_RANDOM_MONSTER_MEDIUM:
        case MP2::OBJ_RANDOM_MONSTER_STRONG:
        case MP2::OBJ_RANDOM_MONSTER_VERY_STRONG:
            assert( isFirstLoad );

            updateRandomMonster( tile );
            updateObjectInfoTile( tile, isFirstLoad );
            return;

        case MP2::OBJ_GENIE_LAMP:
            // The number of Genies is set when loading the map and does not change anymore
            if ( isFirstLoad ) {
                setMonsterCountOnTile( tile, Rand::Get( 2, 4 ) );
            }
            break;

        case MP2::OBJ_WATCH_TOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREE_HOUSE:
        case MP2::OBJ_ARCHER_HOUSE:
        case MP2::OBJ_GOBLIN_HUT:
        case MP2::OBJ_DWARF_COTTAGE:
        case MP2::OBJ_HALFLING_HOLE:
        case MP2::OBJ_PEASANT_HUT:
        // recruit dwelling
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREE_CITY:
        case MP2::OBJ_WAGON_CAMP:
        case MP2::OBJ_DESERT_TENT:
        case MP2::OBJ_TROLL_BRIDGE:
        case MP2::OBJ_DRAGON_CITY:
        case MP2::OBJ_CITY_OF_DEAD:
        case MP2::OBJ_WATER_ALTAR:
        case MP2::OBJ_AIR_ALTAR:
        case MP2::OBJ_FIRE_ALTAR:
        case MP2::OBJ_EARTH_ALTAR:
        case MP2::OBJ_BARROW_MOUNDS:
            updateDwellingPopulationOnTile( tile, isFirstLoad );
            break;
        default:
            break;
        }
    }

    void updateRandomArtifact( Tiles & tile )
    {
        Artifact art;

        switch ( tile.GetObject() ) {
        case MP2::OBJ_RANDOM_ARTIFACT:
            art = Artifact::Rand( Artifact::ART_LEVEL_ALL_NORMAL );
            break;
        case MP2::OBJ_RANDOM_ARTIFACT_TREASURE:
            art = Artifact::Rand( Artifact::ART_LEVEL_TREASURE );
            break;
        case MP2::OBJ_RANDOM_ARTIFACT_MINOR:
            art = Artifact::Rand( Artifact::ART_LEVEL_MINOR );
            break;
        case MP2::OBJ_RANDOM_ARTIFACT_MAJOR:
            art = Artifact::Rand( Artifact::ART_LEVEL_MAJOR );
            break;
        default:
            return;
        }

        if ( !art.isValid() ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Failed to set an artifact over a random artifact on tile " << tile.GetIndex() )
            return;
        }

        tile.SetObject( MP2::OBJ_ARTIFACT );

        uint32_t uidArtifact = tile.getObjectIdByObjectIcnType( MP2::OBJ_ICN_TYPE_OBJNARTI );
        if ( uidArtifact == 0 ) {
            uidArtifact = tile.GetObjectUID();
        }

        static_assert( std::is_same_v<decltype( Maps::Tiles::updateTileById ), void( Tiles &, uint32_t, uint8_t )>,
                       "Type of updateTileById() has been changed, check the logic below" );

        const uint32_t artSpriteIndex = art.IndexSprite();
        assert( artSpriteIndex > std::numeric_limits<uint8_t>::min() && artSpriteIndex <= std::numeric_limits<uint8_t>::max() );

        Maps::Tiles::updateTileById( tile, uidArtifact, static_cast<uint8_t>( artSpriteIndex ) );

        // replace artifact shadow
        if ( Maps::isValidDirection( tile.GetIndex(), Direction::LEFT ) ) {
            Maps::Tiles::updateTileById( world.GetTiles( Maps::GetDirectionIndex( tile.GetIndex(), Direction::LEFT ) ), uidArtifact,
                                         static_cast<uint8_t>( artSpriteIndex - 1 ) );
        }
    }

    void updateRandomResource( Tiles & tile )
    {
        tile.SetObject( MP2::OBJ_RESOURCE );

        const uint8_t resourceSprite = Resource::GetIndexSprite( Resource::Rand( true ) );

        uint32_t uidResource = tile.getObjectIdByObjectIcnType( MP2::OBJ_ICN_TYPE_OBJNRSRC );
        if ( uidResource == 0 ) {
            uidResource = tile.GetObjectUID();
        }

        Maps::Tiles::updateTileById( tile, uidResource, resourceSprite );

        // Replace shadow of the resource.
        if ( Maps::isValidDirection( tile.GetIndex(), Direction::LEFT ) ) {
            assert( resourceSprite > 0 );
            Maps::Tiles::updateTileById( world.GetTiles( Maps::GetDirectionIndex( tile.GetIndex(), Direction::LEFT ) ), uidResource, resourceSprite - 1 );
        }
    }

    void updateRandomMonster( Tiles & tile )
    {
        Monster mons;

        switch ( tile.GetObject() ) {
        case MP2::OBJ_RANDOM_MONSTER:
            mons = Monster::Rand( Monster::LevelType::LEVEL_ANY );
            break;
        case MP2::OBJ_RANDOM_MONSTER_WEAK:
            mons = Monster::Rand( Monster::LevelType::LEVEL_1 );
            break;
        case MP2::OBJ_RANDOM_MONSTER_MEDIUM:
            mons = Monster::Rand( Monster::LevelType::LEVEL_2 );
            break;
        case MP2::OBJ_RANDOM_MONSTER_STRONG:
            mons = Monster::Rand( Monster::LevelType::LEVEL_3 );
            break;
        case MP2::OBJ_RANDOM_MONSTER_VERY_STRONG:
            mons = Monster::Rand( Monster::LevelType::LEVEL_4 );
            break;
        default:
            break;
        }

        tile.SetObject( MP2::OBJ_MONSTER );

        using TileImageIndexType = decltype( tile.GetObjectSpriteIndex() );
        static_assert( std::is_same_v<TileImageIndexType, uint8_t>, "Type of GetObjectSpriteIndex() has been changed, check the logic below" );

        assert( mons.GetID() > std::numeric_limits<TileImageIndexType>::min() && mons.GetID() <= std::numeric_limits<TileImageIndexType>::max() );

        tile.setObjectSpriteIndex( static_cast<TileImageIndexType>( mons.GetID() - 1 ) ); // ICN::MONS32 starts from PEASANT
    }

    void updateMonsterInfoOnTile( Tiles & tile )
    {
        const Monster mons = Monster( tile.GetObjectSpriteIndex() + 1 ); // ICN::MONS32 start from PEASANT
        uint32_t count = 0;

        // update count (mp2 format)
        if ( tile.GetQuantity1() || tile.GetQuantity2() ) {
            count = tile.GetQuantity2();
            count <<= 8;
            count |= tile.GetQuantity1();
            count >>= 3;
        }

        setMonsterOnTile( tile, mons, count );
    }

    void setMonsterOnTile( Tiles & tile, const Monster & mons, const uint32_t count )
    {
        tile.SetObject( MP2::OBJ_MONSTER );

        // If there was another object sprite here (shadow for example) push it down to Addons,
        // except when there is already MONS32.ICN here.
        if ( tile.getObjectIcnType() != MP2::OBJ_ICN_TYPE_UNKNOWN && tile.getObjectIcnType() != MP2::OBJ_ICN_TYPE_MONS32 && tile.GetObjectSpriteIndex() != 255 ) {
            // Push object sprite to Level 1 Addons preserving the Layer Type.
            tile.AddonsPushLevel1( TilesAddon( tile.getLayerType(), tile.GetObjectUID(), tile.getObjectIcnType(), tile.GetObjectSpriteIndex(), false, false ) );

            // Set unique UID for placed monster.
            tile.setObjectUID( World::GetUniq() );
            tile.setObjectIcnType( MP2::OBJ_ICN_TYPE_MONS32 );
        }

        using TileImageIndexType = decltype( tile.GetObjectSpriteIndex() );
        static_assert( std::is_same_v<TileImageIndexType, uint8_t>, "Type of GetObjectSpriteIndex() has been changed, check the logic below" );

        const uint32_t monsSpriteIndex = mons.GetSpriteIndex();
        assert( monsSpriteIndex >= std::numeric_limits<TileImageIndexType>::min() && monsSpriteIndex <= std::numeric_limits<TileImageIndexType>::max() );

        tile.setObjectSpriteIndex( static_cast<TileImageIndexType>( monsSpriteIndex ) );

        const bool setDefinedCount = ( count > 0 );

        if ( setDefinedCount ) {
            setMonsterCountOnTile( tile, count );
        }
        else {
            setMonsterCountOnTile( tile, mons.GetRNDSize() );
        }

        if ( mons.GetID() == Monster::GHOST || mons.isElemental() ) {
            // Ghosts and elementals never join hero's army.
            setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_SKIP );
        }
        else if ( setDefinedCount || ( world.GetWeekType().GetType() == WeekName::MONSTERS && world.GetWeekType().GetMonster() == mons.GetID() ) ) {
            // Wandering monsters with the number of units specified by the map designer are always considered as "hostile" and always join only for money.

            // Monsters will be willing to join for some amount of money.
            setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_MONEY );
        }
        else {
            // 20% chance for join
            if ( 3 > Rand::Get( 1, 10 ) ) {
                setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_FREE );
            }
            else {
                setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_MONEY );
            }
        }
    }
}
