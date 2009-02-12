#ifndef ALPHABET_H
#define ALPHABET_H

#include <ustring/UString.h>
#include<string>
#include <ostream>

using namespace sf1lib;
using namespace std;

/**
 *@class AlphabetGenerator
 *@brief It is to generate alphabet in an unsigned short array.
 *
 * Take english charactors for example. The input string is 'abcdefghijklmnopqrstuvwxyz'.
 *Then, it will print an sorted unsigned short array for the input in console.
 */
template
<
  UString::EncodingType ENCODE_TYPE
>
class AlphabetGenerator
{
public:
  
  AlphabetGenerator(const string& alp)
  {
    // UString s(alp, UString::UTF_8);
//     string str;
//     s.convertString(str, ENCODE_TYPE);
    
    pUstr_ = new UString(alp, ENCODE_TYPE);
    sort();
  }

  /**
   *It prints the sorted alphabet in console
   **/
  friend ostream& operator << ( ostream& os, const AlphabetGenerator& alp)
  {
    //alp.pUstr_->displayStringValue(ENCODE_TYPE, os);
    
    os<<"[";
    for (size_t i=0; i<alp.pUstr_->length()-1; i++)
      os<<(*alp.pUstr_)[i]<<",";

    os<<(*alp.pUstr_)[alp.pUstr_->length()-1]<<"]\n";

    os<<"TOTAL:"<<alp.pUstr_->length()<<endl;
    
    return os;
  }
  
protected:
  UString* pUstr_;

  /**
   *It sorts the alphabet in increasing order.
   **/
  void sort()
  {
    for (size_t i=0; i<pUstr_->length()-1; i++)
    {
      UCS2Char min = (*pUstr_)[i];
      size_t idx = i;
      
      for (size_t j=i+1; j<pUstr_->length(); j++)
      {
        if (min>(*pUstr_)[j])
        {
          idx = j;
          min = (*pUstr_)[j];
        }
      }

      if (idx == i)
        continue;

      (*pUstr_)[idx] = (*pUstr_)[i];
      (*pUstr_)[i] = min;
    }
    
  }
}
;

// static const unsigned int a2z_size = 26;
// static unsigned short a2z[a2z_size] = 
//   {97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122
//   };

static const unsigned int a2z_size = 63;//!< The alphabet size.
static unsigned short a2z[a2z_size] = 
  {48,49,50,51,52,53,54,55,56,57,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,95,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122
  };//!< The alphabet.

//   {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
//   };


// static const unsigned int a2z_size = 3755;
// static unsigned short a2z[a2z_size] = {19968,19969,19971,19975,19976,19977,19978,19979,19981,19982,19985,19987,19988,19990,19992,19993,19994,19995,19996,19997,20002,20004,20005,20007,20010,20011,20013,20016,20018,20020,20024,20025,20026,20027,20029,20030,20035,20037,20040,20041,20043,20044,20045,20046,20047,20048,20050,20051,20052,20054,20056,20057,20061,20062,20063,20064,20065,20070,20080,20081,20083,20094,20102,20104,20105,20107,20108,20110,20111,20113,20114,20116,20117,20122,20123,20129,20130,20132,20133,20134,20135,20136,20137,20139,20140,20141,20142,20146,20154,20159,20160,20161,20165,20166,20167,20170,20171,20173,20174,20177,20179,20180,20181,20182,20183,20184,20185,20191,20195,20196,20197,20202,20204,20208,20210,20214,20215,20219,20221,20223,20225,20234,20237,20238,20239,20240,20241,20247,20248,20249,20250,20254,20255,20256,20260,20262,20266,20271,20272,20276,20278,20280,20282,20284,20291,20294,20301,20302,20303,20304,20305,20307,20309,20313,20315,20316,20320,20323,20329,20332,20335,20336,20339,20351,20356,20360,20363,20365,20375,20379,20381,20384,20387,20389,20390,20391,20392,20393,20398,20399,20405,20415,20419,20420,20426,20431,20432,20439,20440,20445,20446,20449,20457,20461,20462,20463,20465,20474,20493,20498,20500,20504,20505,20506,20511,20513,20518,20522,20538,20540,20542,20551,20559,20570,20572,20581,20598,20599,20607,20608,20613,20616,20621,20643,20648,20652,20658,20667,20687,20698,20711,20723,20725,20731,20754,20769,20799,20801,20803,20804,20805,20806,20808,20809,20811,20813,20817,20820,20826,20828,20834,20837,20840,20843,20844,20845,20848,20849,20851,20852,20853,20854,20855,20856,20857,20859,20860,20861,20864,20869,20872,20873,20876,20877,20882,20885,20887,20889,20891,20892,20896,20900,20908,20911,20912,20914,20915,20917,20918,20919,20923,20928,20932,20934,20937,20939,20940,20943,20945,20955,20957,20960,20961,20964,20973,20975,20976,20979,20982,20984,20985,20986,20987,20989,20991,20992,20993,20995,20998,20999,21002,21009,21010,21015,21016,21017,21018,21019,21021,21024,21028,21032,21033,21035,21038,21040,21046,21047,21048,21049,21050,21051,21053,21057,21058,21059,21066,21069,21072,21073,21076,21078,21093,21095,21097,21098,21103,21106,21119,21128,21147,21149,21150,21151,21152,21153,21155,21160,21161,21162,21163,21169,21170,21171,21183,21187,21191,21193,21195,21202,21208,21215,21220,21242,21246,21247,21248,21253,21254,21256,21270,21271,21273,21277,21280,21281,21283,21290,21305,21306,21307,21311,21313,21315,21319,21320,21321,21322,21326,21327,21329,21330,21331,21333,21334,21335,21338,21340,21342,21344,21345,21346,21348,21351,21355,21359,21360,21361,21363,21364,21365,21367,21368,21375,21378,21380,21381,21382,21385,21387,21388,21397,21400,21402,21407,21410,21414,21416,21417,21435,21439,21441,21442,21448,21449,21450,21451,21452,21453,21457,21460,21462,21463,21464,21465,21467,21472,21475,21476,21477,21478,21482,21483,21484,21485,21486,21487,21488,21490,21491,21494,21495,21496,21497,21500,21505,21507,21508,21512,21513,21514,21516,21517,21518,21519,21520,21521,21523,21525,21527,21531,21533,21534,21535,21536,21542,21543,21544,21545,21547,21548,21549,21550,21551,21553,21556,21557,21560,21561,21563,21564,21566,21568,21574,21576,21578,21584,21589,21592,21595,21596,21602,21608,21619,21621,21624,21627,21628,21629,21632,21638,21643,21644,21646,21647,21648,21650,21653,21654,21657,21672,21676,21679,21681,21683,21688,21693,21696,21697,21700,21702,21703,21704,21705,21709,21710,21713,21719,21727,21733,21734,21736,21737,21738,21741,21742,21746,21754,21756,21761,21766,21767,21769,21776,21796,21804,21806,21807,21809,21822,21827,21828,21830,21834,21857,21860,21861,21862,21866,21870,21880,21884,21888,21890,21892,21895,21897,21898,21912,21916,21917,21927,21939,21943,21947,21957,21971,21980,21985,21987,22013,22025,22030,22040,22043,22065,22066,22068,22070,22075,22079,22094,22120,22122,22124,22134,22158,22159,22179,22199,22204,22218,22234,22235,22238,22240,22242,22244,22253,22256,22257,22260,22266,22269,22270,22275,22278,22280,22303,22307,22312,22317,22320,22330,22334,22336,22343,22346,22349,22350,22351,22352,22353,22359,22362,22363,22365,22366,22367,22368,22369,22372,22374,22378,22383,22391,22402,22403,22404,22411,22418,22427,22434,22435,22438,22443,22446,22466,22467,22475,22478,22484,22495,22496,22521,22522,22530,22534,22545,22549,22561,22564,22570,22576,22581,22604,22609,22612,22616,22622,22635,22659,22661,22674,22675,22681,22686,22687,22696,22697,22721,22741,22756,22763,22764,22766,22768,22771,22774,22777,22788,22791,22797,22799,22805,22806,22810,22812,22815,22823,22825,22826,22827,22830,22831,22833,22836,22839,22840,22841,22842,22852,22855,22856,22857,22859,22862,22863,22865,22868,22870,22871,22880,22882,22885,22899,22900,22902,22904,22905,22909,22914,22916,22918,22919,22920,22922,22930,22931,22934,22937,22949,22952,22958,22969,22971,22982,22987,22992,22993,22995,22996,23002,23004,23013,23016,23020,23035,23039,23041,23043,23044,23047,23064,23068,23071,23072,23077,23081,23089,23094,23110,23113,23130,23146,23156,23158,23167,23186,23194,23219,23233,23234,23241,23244,23265,23273,23376,23380,23381,23383,23384,23385,23388,23389,23391,23395,23396,23398,23401,23402,23408,23413,23418,23421,23425,23427,23429,23431,23432,23433,23435,23436,23439,23447,23448,23449,23450,23451,23452,23453,23454,23456,23457,23458,23459,23460,23462,23466,23467,23472,23475,23476,23477,23478,23481,23485,23486,23487,23490,23492,23493,23494,23495,23500,23504,23506,23507,23517,23518,23519,23521,23525,23528,23544,23545,23546,23547,23548,23551,23553,23556,23558,23561,23562,23567,23569,23572,23574,23576,23578,23581,23588,23591,23601,23608,23609,23610,23612,23613,23614,23615,23616,23617,23618,23621,23624,23625,23626,23627,23630,23631,23633,23637,23646,23648,23649,23653,23663,23665,23673,23679,23681,23682,23700,23703,23707,23721,23725,23731,23736,23743,23769,23777,23782,23784,23786,23789,23792,23803,23815,23822,23828,23830,23849,23853,23884,24013,24029,24030,24033,24034,24037,24038,24039,24040,24041,24043,24046,24049,24050,24051,24052,24055,24062,24065,24066,24067,24069,24070,24072,24076,24080,24085,24086,24088,24090,24091,24092,24093,24102,24103,24109,24110,24120,24125,24130,24133,24140,24149,24162,24178,24179,24180,24182,24184,24187,24188,24189,24191,24196,24198,24199,24202,24207,24208,24211,24212,24213,24215,24217,24218,24220,24222,24223,24230,24231,24237,24246,24247,24248,24265,24266,24275,24278,24310,24311,24314,24320,24322,24323,24324,24330,24335,24339,24341,24343,24344,24347,24351,24352,24357,24358,24359,24367,24369,24377,24378,24402,24403,24405,24413,24418,24420,24422,24425,24426,24428,24429,24432,24433,24441,24443,24444,24448,24449,24452,24453,24456,24458,24459,24464,24466,24471,24472,24481,24490,24494,24503,24509,24515,24517,24518,24524,24525,24535,24536,24537,24544,24551,24555,24561,24565,24571,24573,24575,24576,24577,24578,24590,24594,24596,24597,24598,24604,24605,24608,24613,24615,24616,24618,24623,24635,24643,24651,24653,24656,24658,24661,24674,24676,24680,24681,24683,24684,24685,24687,24688,24691,24694,24700,24703,24708,24713,24717,24724,24735,24736,24739,24742,24744,24748,24751,24754,24760,24764,24773,24778,24779,24785,24789,24796,24799,24800,24806,24807,24808,24809,24811,24813,24814,24815,24816,24819,24822,24825,24826,24833,24840,24841,24847,24858,24863,24868,24871,24895,24904,24908,24910,24913,24917,24930,24935,24936,24944,24951,24971,24974,25000,25022,25026,25032,25034,25042,25062,25096,25098,25100,25101,25102,25103,25104,25105,25106,25110,25112,25114,25130,25134,25139,25140,25143,25151,25152,25153,25159,25163,25165,25166,25169,25170,25171,25172,25176,25179,25187,25190,25191,25193,25195,25196,25197,25198,25199,25200,25203,25206,25209,25212,25214,25215,25216,25220,25225,25226,25233,25234,25235,25237,25238,25239,25240,25242,25243,25248,25249,25250,25252,25253,25256,25259,25260,25265,25269,25273,25276,25277,25279,25282,25284,25285,25286,25287,25288,25289,25292,25293,25294,25296,25298,25299,25300,25302,25304,25305,25307,25308,25311,25314,25315,25317,25318,25319,25320,25321,25324,25325,25327,25329,25331,25332,25335,25340,25341,25342,25343,25345,25346,25351,25353,25358,25361,25366,25370,25371,25373,25374,25375,25376,25377,25379,25380,25381,25384,25386,25387,25391,25402,25405,25410,25413,25414,25417,25420,25421,25422,25423,25424,25429,25438,25439,25441,25442,25443,25447,25454,25462,25463,25467,25472,25474,25479,25480,25481,25484,25487,25488,25490,25494,25496,25504,25506,25507,25509,25511,25512,25513,25514,25523,25527,25528,25530,25545,25549,25551,25552,25554,25558,25569,25571,25577,25578,25581,25588,25597,25600,25601,25602,25605,25615,25616,25619,25620,25628,25630,25642,25644,25645,25658,25661,25668,25670,25671,25672,25674,25684,25688,25703,25705,25720,25721,25730,25733,25735,25745,25746,25749,25758,25764,25769,25772,25773,25774,25776,25781,25788,25794,25797,25805,25806,25810,25822,25830,25856,25874,25880,25899,25903,25910,25913,25915,25918,25919,25925,25928,25932,25935,25937,25942,25945,25947,25949,25950,25954,25955,25958,25964,25968,25970,25972,25975,25991,25995,25996,26001,26007,26009,26012,26015,26017,26020,26021,26023,26025,26029,26031,26032,26041,26045,26049,26053,26059,26063,26071,26080,26082,26085,26086,26087,26088,26089,26092,26093,26097,26102,26103,26106,26114,26118,26124,26126,26127,26131,26132,26143,26144,26149,26151,26152,26157,26159,26172,26174,26179,26187,26188,26194,26195,26197,26202,26212,26214,26216,26222,26223,26224,26228,26230,26234,26238,26242,26247,26257,26262,26263,26286,26292,26329,26333,26352,26354,26355,26356,26361,26364,26366,26367,26368,26376,26377,26379,26381,26388,26391,26395,26397,26399,26408,26410,26411,26412,26413,26415,26417,26420,26421,26426,26429,26432,26434,26435,26438,26441,26446,26447,26448,26449,26454,26460,26463,26464,26465,26469,26472,26477,26479,26480,26494,26495,26497,26500,26505,26512,26517,26519,26522,26524,26525,26530,26531,26538,26539,26543,26550,26551,26564,26575,26576,26577,26578,26579,26580,26588,26590,26592,26597,26604,26607,26609,26611,26612,26623,26629,26631,26632,26635,26639,26641,26643,26646,26647,26657,26666,26679,26680,26681,26684,26685,26690,26691,26693,26694,26696,26700,26704,26705,26707,26708,26723,26725,26728,26729,26742,26753,26757,26758,26775,26786,26790,26791,26792,26797,26799,26800,26803,26816,26825,26827,26829,26834,26837,26840,26842,26848,26862,26865,26869,26874,26885,26893,26894,26898,26925,26928,26941,26943,26964,26970,26974,26999,27004,27010,27014,27028,27036,27048,27060,27063,27088,27099,27133,27146,27167,27169,27178,27185,27207,27225,27233,27249,27264,27268,27308,27424,27425,27426,27427,27431,27442,27450,27454,27463,27465,27468,27490,27491,27492,27493,27494,27495,27498,27513,27515,27516,27523,27526,27529,27530,27531,27542,27572,27573,27575,27583,27585,27589,27595,27597,27599,27602,27604,27605,27606,27607,27609,27611,27617,27627,27631,27663,27665,27667,27668,27670,27675,27679,27682,27686,27687,27688,27694,27695,27696,27700,27704,27712,27713,27714,27719,27721,27728,27733,27735,27739,27741,27742,27743,27744,27745,27748,27754,27760,27762,27769,27773,27774,27777,27778,27779,27784,27785,27791,27801,27803,27807,27809,27812,27813,27814,27815,27818,27819,27822,27827,27832,27833,27835,27836,27837,27838,27839,27844,27845,27849,27850,27852,27861,27867,27870,27873,27874,27875,27877,27880,27882,27888,27891,27893,27899,27900,27901,27905,27915,27922,27927,27931,27934,27941,27946,27953,27954,27963,27964,27965,27966,27969,27973,27974,27975,27978,27979,27982,27985,27987,27993,27994,28006,28009,28010,28014,28020,28023,28024,28034,28037,28040,28041,28044,28046,28053,28059,28061,28063,28065,28067,28068,28070,28071,28072,28073,28074,28079,28082,28085,28088,28096,28100,28102,28107,28108,28113,28118,28120,28129,28132,28139,28140,28142,28145,28147,28151,28153,28155,28165,28170,28173,28176,28180,28183,28189,28192,28193,28195,28196,28201,28205,28207,28212,28216,28218,28227,28237,28246,28248,28251,28286,28287,28291,28293,28297,28304,28316,28322,28330,28335,28342,28346,28353,28359,28363,28369,28371,28372,28378,28382,28385,28388,28389,28390,28392,28393,28404,28418,28422,28431,28435,28436,28448,28459,28465,28467,28478,28493,28504,28508,28510,28518,28525,28526,28548,28552,28558,28572,28577,28595,28608,28626,28689,28748,28779,28781,28783,28784,28789,28790,28792,28796,28798,28799,28809,28810,28814,28818,28820,28821,28825,28844,28845,28846,28847,28851,28856,28857,28860,28861,28865,28866,28867,28872,28888,28889,28891,28895,28900,28902,28903,28905,28907,28908,28909,28911,28919,28921,28925,28937,28938,28949,28953,28954,28966,28976,28982,29004,29006,29022,29028,29031,29038,29053,29060,29066,29071,29076,29081,29087,29100,29123,29134,29141,29157,29190,29226,29228,29233,29237,29238,29239,29240,29241,29245,29255,29256,29260,29273,29275,29279,29281,29282,29287,29289,29298,29301,29305,29306,29312,29313,29322,29356,29359,29366,29369,29378,29380,29384,29392,29399,29401,29406,29408,29409,29420,29421,29422,29424,29425,29432,29436,29454,29462,29467,29468,29481,29482,29483,29486,29492,29502,29503,29549,29572,29575,29577,29579,29590,29595,29609,29611,29615,29616,29618,29627,29642,29645,29648,29664,29677,29699,29701,29702,29705,29712,29730,29747,29748,29749,29750,29756,29786,29790,29791,29808,29814,29827,29916,29922,29923,29924,29926,29934,29942,29943,29956,29976,29978,29980,29983,29989,29992,29993,29995,29997,30000,30001,30002,30003,30005,30007,30008,30011,30021,30028,30031,30036,30041,30044,30053,30054,30058,30068,30072,30086,30095,30097,30103,30105,30106,30111,30113,30116,30117,30123,30126,30127,30130,30133,30137,30140,30141,30142,30149,30151,30152,30153,30154,30162,30164,30165,30168,30171,30174,30178,30186,30192,30196,30201,30209,30239,30244,30246,30249,30250,30251,30260,30264,30284,30307,30328,30331,30333,30334,30338,30340,30342,30343,30347,30353,30358,30382,30385,30399,30402,30405,30406,30408,30410,30414,30415,30416,30417,30418,30420,30422,30423,30424,30427,30431,30446,30447,30450,30452,30456,30460,30462,30465,30473,30475,30495,30496,30504,30505,30511,30518,30519,30522,30524,30528,30529,30555,30561,30563,30566,30571,30572,30585,30596,30597,30606,30610,30629,30631,30633,30634,30636,30643,30651,30679,30683,30690,30691,30693,30697,30699,30701,30702,30707,30717,30718,30719,30721,30722,30732,30733,30738,30740,30742,30746,30759,30768,30772,30775,30776,30782,30784,30789,30802,30805,30813,30827,30828,30830,30839,30844,30857,30860,30861,30862,30865,30871,30872,30879,30887,30896,30897,30899,30900,30910,30913,30917,30922,30923,30928,30933,30952,30967,30970,30977,31034,31036,31038,31041,31048,31062,31069,31070,31071,31077,31080,31085,31095,31096,31105,31108,31119,31161,31163,31165,31166,31168,31169,31171,31174,31177,31179,31181,31185,31186,31192,31199,31204,31206,31207,31209,31215,31216,31224,31227,31229,31232,31243,31245,31246,31255,31258,31264,31283,31291,31292,31293,31295,31302,31319,31348,31350,31351,31354,31359,31361,31363,31364,31373,31377,31378,31382,31383,31384,31388,31389,31391,31397,31423,31435,31446,31449,31454,31455,31456,31459,31461,31469,31471,31481,31487,31494,31499,31505,31508,31515,31526,31528,31532,31546,31548,31561,31563,31567,31568,31569,31570,31572,31574,31579,31607,31609,31614,31616,31629,31636,31637,31639,31649,31657,31661,31665,31686,31687,31699,31705,31713,31726,31729,31735,31751,31783,31807,31821,31859,31867,31869,31881,31890,31893,31895,31896,31903,31908,31909,31914,31918,31921,31923,31929,31934,31946,31957,31958,31961,31964,31967,31968,31983,31995,32010,32032,32034,32039,32043,32047,32110,32321,32386,32416,32418,32420,32422,32423,32426,32427,32428,32431,32433,32434,32435,32437,32438,32439,32440,32441,32442,32445,32447,32451,32452,32453,32454,32455,32456,32458,32461,32462,32463,32465,32466,32467,32469,32472,32473,32474,32476,32477,32478,32479,32482,32483,32485,32486,32487,32489,32490,32493,32496,32499,32500,32501,32503,32504,32508,32509,32511,32512,32516,32517,32518,32521,32526,32531,32532,32533,32534,32536,32538,32541,32544,32552,32553,32558,32564,32568,32570,32592,32593,32597,32599,32602,32610,32617,32618,32622,32626,32650,32652,32654,32660,32666,32670,32673,32676,32697,32701,32705,32709,32716,32724,32728,32735,32736,32752,32753,32763,32764,32768,32769,32771,32773,32780,32781,32784,32789,32791,32792,32793,32810,32819,32822,32824,32827,32829,32831,32834,32842,32843,32844,32852,32856,32858,32874,32899,32900,32902,32903,32905,32907,32908,32918,32920,32922,32923,32925,32928,32929,32930,32932,32933,32937,32938,32942,32943,32946,32954,32958,32959,32960,32961,32963,32966,32972,32974,32982,32986,32988,32990,32993,33007,33008,33011,33014,33016,33018,33021,33026,33030,33033,33034,33039,33040,33041,33043,33046,33050,33071,33073,33080,33086,33094,33098,33099,33104,33105,33108,33109,33125,33134,33136,33145,33146,33147,33150,33151,33152,33162,33167,33176,33179,33180,33181,33192,33203,33216,33218,33219,33222,33251,33258,33261,33267,33268,33275,33276,33280,33285,33286,33292,33293,33298,33300,33308,33310,33311,33322,33324,33328,33329,33333,33334,33335,33337,33351,33368,33391,33392,33394,33395,33402,33406,33410,33419,33421,33426,33436,33437,33445,33446,33452,33453,33455,33457,33459,33465,33469,33479,33485,33487,33489,33492,33495,33499,33502,33503,33509,33510,33515,33519,33521,33529,33537,33538,33539,33540,33541,33550,33575,33576,33579,33580,33589,33590,33592,33593,33606,33609,33616,33618,33620,33626,33633,33635,33636,33639,33643,33647,33655,33670,33673,33678,33707,33713,33714,33719,33721,33725,33735,33738,33740,33743,33756,33760,33769,33777,33778,33796,33804,33805,33806,33821,33828,33829,33831,33832,33853,33879,33883,33889,33891,33899,33900,33905,33909,33922,33931,33945,33948,33970,33976,33988,33993,34001,34006,34013,34015,34028,34065,34067,34071,34074,34081,34091,34092,34103,34108,34109,34121,34122,34164,34174,34180,34203,34218,34223,34249,34255,34256,34261,34276,34281,34299,34321,34360,34382,34383,34384,34385,34394,34398,34411,34417,34425,34429,34430,34432,34433,34434,34442,34444,34453,34460,34468,34496,34502,34503,34506,34507,34516,34521,34523,34532,34542,34544,34553,34558,34560,34562,34578,34581,34583,34584,34588,34593,34631,34633,34638,34647,34676,34678,34701,34719,34746,34809,34837,34850,34880,34885,34892,34893,34900,34903,34905,34913,34915,34917,34920,34923,34924,34928,34935,34945,34948,34955,34957,34962,34966,34972,34987,34989,34993,35009,35010,35013,35028,35029,35033,35044,35059,35060,35064,35065,35074,35088,35090,35109,35114,35140,35167,35199,35201,35206,35265,35266,35268,35269,35270,35272,35273,35282,35299,35302,35328,35449,35465,35466,35475,35686,35692,35745,35746,35747,35748,35749,35752,35753,35755,35757,35758,35759,35760,35762,35763,35766,35768,35769,35770,35772,35773,35774,35775,35776,35777,35780,35781,35782,35784,35785,35786,35788,35789,35793,35797,35799,35802,35803,35805,35806,35809,35810,35811,35813,35814,35815,35819,35820,35821,35823,35825,35826,35828,35829,35831,35832,35834,35835,35837,35838,35841,35843,35845,35846,35848,35850,35851,35853,35854,35856,35859,35863,35866,35868,35874,35875,35876,35878,35880,35881,35884,35885,35888,35889,35892,35895,35905,35910,35916,35937,35938,35946,35947,35961,35962,35977,35980,36125,36126,36127,36129,36130,36131,36132,36133,36134,36135,36136,36137,36138,36139,36140,36141,36142,36143,36144,36145,36148,36149,36151,36152,36153,36154,36156,36158,36159,36161,36162,36163,36164,36170,36171,36172,36174,36175,36176,36180,36182,36184,36186,36187,36190,36192,36193,36194,36195,36196,36198,36203,36208,36212,36213,36214,36215,36225,36229,36234,36235,36255,36259,36275,36276,36286,36291,36299,36300,36305,36317,36319,36328,36330,36335,36339,36341,36346,36362,36364,36367,36382,36386,36393,36394,36420,36424,36427,36454,36460,36461,36466,36479,36481,36487,36523,36524,36527,36530,36538,36710,36711,36712,36713,36716,36718,36719,36720,36724,36731,36733,36735,36739,36741,36742,36744,36745,36746,36752,36753,36755,36757,36758,36759,36761,36763,36764,36766,36767,36771,36776,36777,36779,36784,36785,36793,36797,36798,36801,36802,36804,36805,36807,36808,36814,36816,36817,36820,36824,36825,36827,36828,36829,36830,36831,36834,36842,36843,36845,36848,36855,36856,36857,36861,36864,36865,36866,36867,36870,36873,36874,36879,36880,36882,36884,36887,36890,36891,36893,36894,36895,36896,36898,36910,36920,36923,36924,36926,36929,36930,36935,36941,36943,36947,36951,36963,36965,36973,36974,36981,36991,36992,37009,37011,37026,37027,37030,37034,37038,37039,37041,37045,37049,37051,37057,37066,37070,37073,37085,37089,37095,37096,37101,37108,37112,37117,37122,37145,37193,37195,37196,37197,37202,37207,37210,37213,37214,37219,37221,37226,37228,37230,37233,37237,37238,37239,37240,37247,37255,37257,37259,37266,37274,37275,37319,37321,37322,37324,37325,37326,37327,37329,37340,37492,38024,38025,38030,38034,38035,38041,38045,38046,38047,38048,38049,38050,38053,38054,38055,38056,38057,38062,38065,38067,38069,38075,38078,38080,38081,38082,38083,38085,38086,38108,38109,38113,38115,38124,38125,38128,38129,38130,38134,38136,38138,38142,38144,38145,38148,38149,38152,38155,38156,38160,38161,38167,38169,38170,38177,38179,38180,38181,38182,38184,38189,38190,38191,38192,38201,38203,38208,38209,38215,38218,38221,38224,38225,38236,38243,38253,38256,38262,38271,38376,38378,38381,38382,38383,38384,38386,38388,38391,38392,38393,38394,38395,38397,38400,38401,38402,38405,38409,38414,38416,38417,38420,38428,38431,38446,38450,38451,38452,38453,38454,38459,38463,38464,38468,38469,38470,38471,38472,38475,38476,38477,38480,38485,38491,38497,38498,38500,38504,38505,38506,38517,38518,38519,38533,38534,38539,38543,38544,38548,38552,38553,38556,38567,38582,38590,38592,38593,38596,38597,38598,38599,38604,38605,38607,38613,38632,38634,38646,38647,38649,38654,38656,38660,38663,38665,38669,38675,38678,38684,38686,38706,38712,38713,38738,38742,38745,38747,38750,38752,38753,38754,38761,38771,38772,38774,38795,38797,38808,38816,38829,38886,38887,38889,38893,38899,38901,38902,39029,39030,39031,39033,39034,39035,39037,39038,39039,39041,39042,39044,39045,39046,39047,39048,39050,39056,39057,39059,39062,39063,39064,39068,39069,39072,39076,39079,39118,39128,39134,39135,39184,39269,39277,39278,39279,39280,39281,39282,39285,39286,39290,39292,39295,39297,39301,39302,39304,39307,39311,39314,39318,39321,39532,39533,39534,39535,39536,39537,39539,39540,39542,39545,39547,39548,39550,39554,39556,39558,39559,39563,39564,39567,39569,39575,39578,39585,39588,39592,39608,39635,39640,39683,39740,39745,39746,39748,39759,39764,40060,40065,40077,40092,40100,40120,40131,40150,40158,40479,40481,40483,40485,40486,40493,40495,40499,40501,40509,40511,40515,40517,40522,40527,40548,40560,40575,40595,40614,40635,40644,40653,40654,40657,40660,40664,40718,40723,40736,40763,40784,40831,40836,40843,40857,40858,40863};

#endif