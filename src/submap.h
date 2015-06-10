#if !defined(SUBMAP_H)
#define SUBMAP_H

#include "mapdata.h"

struct spawn_point {
 int posx, posy;
 int count;
 std::string type;
 int faction_id;
 int mission_id;
 bool friendly;
 std::string name;
 spawn_point(std::string T = "mon_null", int C = 0, int X = -1, int Y = -1,
             int FAC = -1, int MIS = -1, bool F = false,
             std::string N = "NONE") :
             posx (X), posy (Y), count (C), type (T), faction_id (FAC),
             mission_id (MIS), friendly (F), name (N) {}
};

struct submap {
    inline trap_id get_trap( const int x, const int y ) const {
        return trp[x][y];
    }

    inline void set_trap( const int x, const int y, trap_id trap ) {
        is_uniform = false;
        trp[x][y] = trap;
    }

    inline furn_id get_furn( const int x, const int y ) const {
        return frn[x][y];
    }

    inline void set_furn( const int x, const int y, furn_id furn ) {
        is_uniform = false;
        frn[x][y] = furn;
    }

    inline ter_id get_ter( const int x, const int y ) const {
        return ter[x][y];
    }

    inline void set_ter( const int x, const int y, ter_id terr ) {
        is_uniform = false;
        ter[x][y] = terr;
    }

    inline int get_radiation( const int x, const int y ) const {
        return rad[x][y];
    }

    void set_radiation( const int x, const int y, const int radiation ) {
        is_uniform = false;
        rad[x][y] = radiation;
    }

    void update_lum_add( item const &i, int const x, int const y ) {
        is_uniform = false;
        if (i.is_emissive() && lum[x][y] < 255) {
            lum[x][y]++;
        }
    }

    void update_lum_rem( item const &i, int const x, int const y ) {
        is_uniform = false;
        if (!i.is_emissive()) {
            return;
        } else if (lum[x][y] && lum[x][y] < 255) {
            lum[x][y]--;
            return;
        }

        // Have to scan through all items to be sure removing i will actally lower
        // the count below 255.
        int count = 0;
        for (auto const &it : itm[x][y]) {
            if (it.is_emissive()) {
                count++;
            }
        }

        if (count <= 256) {
            lum[x][y] = static_cast<uint8_t>(count - 1);
        }
    }

    bool has_graffiti( int x, int y ) const;
    const std::string &get_graffiti( int x, int y ) const;
    void set_graffiti( int x, int y, const std::string &new_graffiti );
    void delete_graffiti( int x, int y );

    // Signage is a pretend union between furniture on a square and stored
    // writing on the square. When both are present, we have signage.
    // Its effect is meant to be cosmetic and atmospheric only.
    inline bool has_signage( const int x, const int y) const {
        if( frn[x][y] == furnfind( "f_sign" ) ) {
            return cosmetics[x][y].find("SIGNAGE") != cosmetics[x][y].end();
        }

        return false;
    }
    // Dependent on furniture + cosmetics.
    inline const std::string get_signage( const int x, const int y ) const {
        if( frn[x][y] == furnfind( "f_sign" ) ) {
            auto iter = cosmetics[x][y].find("SIGNAGE");
            if( iter != cosmetics[x][y].end() ) {
                return iter->second;
            }
        }

        return "";
    }
    // Can be used anytime (prevents code from needing to place sign first.)
    inline void set_signage( const int x, const int y, std::string s) {
        is_uniform = false;
        cosmetics[x][y]["SIGNAGE"] = s;
    }
    // Can be used anytime (prevents code from needing to place sign first.)
    inline void delete_signage( const int x, const int y) {
        is_uniform = false;
        cosmetics[x][y].erase("SIGNAGE");
    }

    // TODO: make trp private once the horrible hack known as editmap is resolved
    ter_id          ter[SEEX][SEEY];  // Terrain on each square
    furn_id         frn[SEEX][SEEY];  // Furniture on each square
    std::uint8_t    lum[SEEX][SEEY];  // Number of items emitting light on each square
    std::list<item> itm[SEEX][SEEY];  // Items on each square
    field           fld[SEEX][SEEY];  // Field on each square
    trap_id         trp[SEEX][SEEY];  // Trap on each square
    int             rad[SEEX][SEEY];  // Irradiation of each square

    // If is_uniform is true, this submap is a solid block of terrain
    // Uniform submaps aren't saved/loaded, because regenerating them is faster
    bool is_uniform;

    std::map<std::string, std::string> cosmetics[SEEX][SEEY]; // Textual "visuals" for each square.

    active_item_cache active_items;

    int field_count = 0;
    int turn_last_touched = 0;
    int temperature = 0;
    std::vector<spawn_point> spawns;
    /**
     * Vehicles on this submap (their (0,0) point is on this submap).
     * This vehicle objects are deleted by this submap when it gets
     * deleted.
     * TODO: submap owns these pointers, they ought to be unique_ptrs.
     */
    std::vector<vehicle*> vehicles;
    computer comp;
    basecamp camp;  // only allowing one basecamp per submap

    submap();
    ~submap();
    // delete vehicles and clear the vehicles vector
    void delete_vehicles();
};

/**
 * A wrapper for a submap point. Allows getting multiple map features
 * (terrain, furniture etc.) without directly accessing submaps or
 * doing multiple bounds checks and submap gets.
 */
struct maptile {
private:
    friend map; // To allow "sliding" the tile in x/y without bounds checks
    friend submap;
    submap *const sm;
    size_t x;
    size_t y;

    maptile( submap *sub, const size_t nx, const size_t ny ) :
        sm( sub ), x( nx ), y( ny ) { }
public:
    inline trap_id get_trap() const
    {
        return sm->get_trap( x, y );
    }

    inline furn_id get_furn() const
    {
        return sm->get_furn( x, y );
    }

    inline ter_id get_ter() const
    {
        return sm->get_ter( x, y );
    }

    inline const trap &get_trap_t() const
    {
        return sm->get_trap( x, y ).obj();
    }

    const furn_t &get_furn_t() const;
    const ter_t &get_ter_t() const;

    inline const field &get_field() const
    {
        return sm->fld[x][y];
    }

    inline field_entry* find_field( const field_id field_to_find )
    {
        return sm->fld[x][y].findField( field_to_find );
    }

    inline bool add_field( const field_id field_to_add, const int new_density, const int new_age )
    {
        const bool ret = sm->fld[x][y].addField( field_to_add, new_density, new_age );
        if( ret ) {
            sm->field_count++;
        }

        return ret;
    }

    inline int get_radiation() const
    {
        return sm->get_radiation( x, y );
    }

    inline bool has_graffiti() const
    {
        return sm->has_graffiti( x, y );
    }

    inline const std::string &get_graffiti() const
    {
        return sm->get_graffiti( x, y );
    }

    inline bool has_signage() const
    {
        return sm->has_signage( x, y );
    }

    inline const std::string get_signage() const
    {
        return sm->get_signage( x, y );
    }

    // For map::draw_maptile
    inline size_t get_item_count() const
    {
        return sm->itm[x][y].size();
    }

    inline const item &get_last_item() const
    {
        return sm->itm[x][y].back();
    }
};

#endif
