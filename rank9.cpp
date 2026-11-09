#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <climits>
#include <queue>
#include <set>

using namespace std;

struct Point { int x, y; };
//minimum heap comparison,getting lowest f score 1st
struct Node {
    int x, y, g, h, f, parX, parY;
    bool operator>(const Node& other) const {
        if (f != other.f)return f > other.f;
        return h > other.h; // tie break
    }
};
struct town { int id, x, y;string desired_connections; };

struct conne { int from; int to; };

vector<Point>pathrec(int targX, int targY, int stX, int stY, const vector<vector<Point>>& parent) {
    vector<Point>path;
    Point curr = { targX, targY };
    while (!(curr.x == stX && curr.y == stY)) {
        path.push_back(curr);
        curr = parent[curr.y][curr.x];
    }
    path.push_back({ stX, stY });
    reverse(path.begin(), path.end());
    return path;
};


bool stillvalid(const vector<Point>& path, const vector<vector<int>>& grid) {
    for(size_t i = 1;i+1<path.size();i++){//skip start and target towns
        const Point& p = path[i];
        if(grid[p.y][p.x]==-1)return false;
    }
    return true;
        
    // for (const Point& p : path) {
    //     if (grid[p.y][p.x] == -1)return false; //cells are inked so towns are never-1. thus safe
    // }
    // return true;
}


vector<Point> astar(int stX, int stY, int targX, int targY, int width, int height, const vector<vector<int>>& grid, const vector<vector<bool>>& hasTrack) {
    // closed list for a visited arr
    vector<vector<bool>> closed(height, vector<bool>(width, false));
    // best g score tracker and parent per cell
    vector<vector<Point>> parent(height, vector<Point>(width, { -1, -1 }));
    vector<vector<int>>gsc(height, vector<int>(width, INT_MAX));
    //start node init
    gsc[stY][stX] = 0;
    int initH = abs(stX - targX) + abs(stY - targY);
    priority_queue<Node, vector<Node>, greater<Node>> opens;
    opens.push({ stX, stY, 0, initH, initH, -1, -1 });

    //Directional movement in terms of priority
        //                N   E  S   W
    const int dx[4] = { 0, 1, 0, -1 };
    const int dy[4] = { -1, 0, 1, 0 };
    while (!opens.empty()) {
        Node curr = opens.top();
        opens.pop();
        if (closed[curr.y][curr.x])continue;
        closed[curr.y][curr.x] = true;
        if (curr.x == targX && curr.y == targY) {
            return pathrec(targX, targY, stX, stY, parent);
        }
        for (int i = 0;i < 4;i++) {
            int newX = curr.x + dx[i];
            int newY = curr.y + dy[i];
            if (newX < 0 || newX >= width || newY < 0 || newY >= height)continue;
            //can go to target town but blocks others towns
            bool targT = (newX == targX && newY == targY);
            if (grid[newY][newX] == -1 && !targT)continue;
            // calc terrain costs
            int tc;
            if (targT) tc = 0;
            // target can be entered w/o adding cost
            else if (hasTrack[newY][newX]) tc = 0;  // FREE to extend existing track!
            else tc = grid[newY][newX] + 1;
            int tbd = gsc[curr.y][curr.x] + tc;
            if (tbd < gsc[newY][newX]) {
                gsc[newY][newX] = tbd;
                parent[newY][newX] = { curr.x, curr.y };
                int h = abs(newX - targX) + abs(newY - targY);
                opens.push({ newX, newY, tbd, h, tbd + h });
            }
        }
    }
    return {};
}

vector<int> connects(const string& s) {
    vector<int>an;
    if (s == "0" || s == "x" || s.empty())return an;
    size_t st = 0;
    while (st < s.size()) {
        size_t comma = s.find(',', st);
        if (comma == string::npos) {
            string token = s.substr(st);
            if (comma == string::npos) { an.push_back(stoi(token)); }break;
        }
        string token = s.substr(st, comma - st);
        if (!token.empty() && token != "x") { an.push_back(stoi(token)); }
        st = comma + 1;
    }
    return an;
}




string exec(vector<Point>& path, const vector<vector<int>>& grid, const vector<vector<bool>>& hasTrack, int& currst) {
    int pp = 3;
    bool pl = false;
    string res;
    while (currst < path.size() - 1 && pp > 0) {
        Point p = path[currst];
        if(hasTrack[p.y][p.x]){currst++;continue;} //already placed, free, dont place
        int c = grid[p.y][p.x] + 1;
        if(pp<c)break;
        pp -= c;
        if (pl)res += ";";
        res += "PLACE_TRACKS " + to_string(p.x) + " " + to_string(p.y);
        pl = true;
        currst++;
    }
    if (!pl) { res = "WAIT"; }
    return res;
}

town getTown(const vector<town>& towns, int id) {
    for (const auto& t : towns) {
        if (t.id == id)return t;
    }
    return towns[0];
}

int main()
{
    int my_id; // 0 or 1
    cin >> my_id; cin.ignore();
    int width; // map size
    cin >> width; cin.ignore();
    int height;
    cin >> height; cin.ignore();

    
    // region area. 
    // if type == 0(i.e. plains, there's a town)
    vector<vector<int>>region(height, vector<int>(width));
    vector<vector<int>> grid(height, vector<int>(width));
    vector<vector<bool>> hasTrack(height, vector<bool>(width, false));
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int region_id;
            int type; // 0 (PLAINS), 1 (RIVER), 2 (MOUNTAIN), 3 (POI)
            cin >> region_id >> type; cin.ignore();
            grid[i][j] = type;
            region[i][j] = region_id;
        }
    }

    //town area
    //each game starts w town thus, towns at start. 1 per region & no 2 regions sharing a border contains a town
    vector<town>towns;

    int town_count;
    cin >> town_count; cin.ignore();
    for (int i = 0; i < town_count; i++) {
        town t;

        //unilateral -> only 1 way. can be 0.
        cin >> t.id >> t.x >> t.y >> t.desired_connections;cin.ignore();
        towns.push_back(t);
        cerr << "DESIRED CONNECTIONS CHECK" << endl << t.desired_connections << endl;

    }
    //changed act_ind from -1 to 0 due to moving the act_ind++ lines into path.empty() loop
    int act_ind = 0, currst = 1;
    vector<Point> path;
    set<int>townRegions;
    set<int>unreach;
    for (const town& t : towns) {
        grid[t.y][t.x] = -1;
        townRegions.insert(region[t.y][t.x]);
    }
    vector<conne>co1;
    for (const town& t : towns) {
        vector<int> req = connects(t.desired_connections);
        // for(int r:req)cerr<<"Town 0 wneeds: "<<r<<endl; debugging check for town connections
        for (int t1 : req) {
            co1.push_back({ t.id, t1 });
        }
    }
    while (1) {
        int my_score;
        cin >> my_score; cin.ignore();
        int foe_score;
        cin >> foe_score; cin.ignore();
        int enem = 1 - my_id;
        int bestx = -1, besty = -1, bestinst = -1;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int tracks_owner;
                int instability; // region inked (destroyed) when this >= 3.
                bool inked; // true if region is destroyed.
                string part_of_active_connections; // if this cell is part of one or more railway connections, this will be town ids (separated by -) in a list separated by commas. e.g. 0-1,1-2,1-3. "x" otherwise.
                cin >> tracks_owner >> instability >> inked >> part_of_active_connections; cin.ignore();
                if (inked) grid[i][j] = -1;
                
                // Track placed rails from part_of_active_connections
               
                    // new fix, verifies no owner value
                    hasTrack[i][j] = (tracks_owner>=0)&&!inked;
               
                
                if (tracks_owner == enem && !inked && !townRegions.count(region[i][j])) {
                    if (instability > bestinst)//keep hindering same region
                    {
                        bestinst = instability;bestx = j;besty = i;
                    }
                }
            }
        }

       
        if(!path.empty()&&!stillvalid(path,grid)){path.clear();currst=1;}
        else if(!path.empty()&& currst>=(int)path.size()-1){act_ind++;path.clear();currst=1;}

        // if (!path.empty() && currst >= path.size() - 1) {
        //     act_ind++;path.clear();currst = 1;
        // }
        // while (path.empty() && act_ind < (int)co1.size() - 1) {
        //     conne c = co1[act_ind];
        //     town fr = getTown(towns, c.from);
        //     town to = getTown(towns, c.to);
        //     path = astar(fr.x, fr.y, to.x, to.y, width, height, grid, hasTrack);
        //     if (path.empty()) {
        //         //unreachable connection since pathfinding fails
        //         unreach.insert(act_ind);
        //         act_ind++;
        //         currst = 1;
        //     }
        //     else {
        //         currst = 1;
        //     }
        // }
        

        string moveact;
        int tries = 0;
        while(tries++<(int)co1.size()+1){
            if(path.empty()&&act_ind < (int)co1.size()){
                 conne c = co1[act_ind];
            town fr = getTown(towns, c.from);
            town to = getTown(towns, c.to);
            path = astar(fr.x, fr.y, to.x, to.y, width, height, grid, hasTrack);
            currst = 1;
            if(path.empty()){
                unreach.insert(act_ind);
                act_ind++;
                continue;
            }
            }
            if(path.empty())break;

            moveact = exec(path,grid,hasTrack,currst);
            if(moveact!="WAIT")break;
            if(currst>=(int)path.size()-1){
                act_ind++;path.clear();currst=1;
            }else break;
        }
        if(moveact.empty())moveact = "WAIT";
        if (bestx != -1) { cout << moveact << ";DISRUPT " << bestx << " " << besty << endl; }
        else { cout << moveact << endl; }
        
        // if (!path.empty() && currst < (int)path.size() - 1) {
        //     moveact = exec(path, grid, hasTrack, currst);
        // }
        // else if (!path.empty()) { moveact = "WAIT"; }
        // else { moveact = "WAIT"; }
        

        }
    
    
    
    







    /* Issues and their fixes
    1. there were 2 progress counters, curr & currst. both global.
    issue - while((path.empty...)) loop checked with currst determing "done w this path, move to next connection"
    - currst only resets to 1 when new path, but never advances with curr. thus, after 1 path, currst stuck at 1 indefinetly
    -> leads to: pick next connection never retriggered(depends on currst, continously looked "not done")
    ->curr goes till end of path via exec which leads execs curr<path.size()-1 to do nothing and print WAIT
    fix: in exec, use currst and remove curr.
    
    2.i) ignored part_of_active_connections to identify placed tracks.
    ii) inking only targetted enemy tracks in non-town regions,
     also picking highest instability cells w/o considerign strategic value
     no tracking of regions close to being inked i.e. instability>=2
     iii)pathfinding: cheaper to extend existing tracks for a*
     no handling of opponent placing tracks on mine or planned path
     path validation only checks 4 inked cells, not blocked paths
     iv)connections are processed in fixed order w/o prioritization
     need plan b incase a connection becomes impossible
     need to find connections that give most points




    */
    // Write an action using cout. DON'T FORGET THE "<< endl"
    // To debug: cerr << "Debug messages..." << endl;
    // AUTOPLACE x1 y1 x2 | PLACE_TRACKS x y | DISRUPT regionId | MESSAGE text
}
