
function song()
{
    var state = {}
    state.bottlesOfBeer = null
    state.bottlesOfBeerOnTheWall = null
    state.takeOneDown = null
    state.createVerse = null
    state.getNormalVerseFunction = null
    state.bottlesOfBeer = function(i)
    {
        var s ='';
        s += '' + i;
        s += ' bottles of beer';
        return s;
    };
    state.bottlesOfBeerOnTheWall = function(i)
    {
        var s = '';
        s += state.bottlesOfBeer(i);
        s += ' on the wall';
        return s;
    };
    state.takeOneDown = function()
    {
        return 'Take one down and pass it around, ';
    };
    state.createVerse = function(first,second)
    {
        println(first, second);
    };
    state.getNormalVerseFunction = function(i)
    {
        return function()
        {
            var s = '';
            var ns = '';
            s += state.bottlesOfBeerOnTheWall(i);
            s += ', ';
            s += state.bottlesOfBeer(i);
            ns += state.takeOneDown();
            ns += state.bottlesOfBeerOnTheWall(i-1);
            ns += '.';
            state.createVerse(s, ns);
        };
    };
    state.verse = [];
    for(var i = 3; i < 100; i++ )
    {
        state.verse[i] = state.getNormalVerseFunction(i);
    }
    state.verse[2] = function()
    {
       state.createVerse(
            state.bottlesOfBeerOnTheWall(2)+', '+state.bottlesOfBeer(2),
            state.takeOneDown()+'1 bottle of beer.'
        );
    };
    state.verse[1] = function()
    {
        state.createVerse(
            '1 bottle of beer on the wall, 1 bottle of beer.',
            state.takeOneDown()+state.bottlesOfBeerOnTheWall('no more')+'.'
        );
    };
    state.verse[0] = function()
    {
        state.createVerse(
            state.bottlesOfBeerOnTheWall('No more')+', '+state.bottlesOfBeer('no more'),
            'Go to the store and buy some more, '+state.bottlesOfBeerOnTheWall(99)+'.'
        );
    };
    state.getDom = function()
    {
        for( var i = 99; i >= 0 ; i-- )
        {
            state.verse[i]();
        }
    };
    return state
};

song().getDom();
