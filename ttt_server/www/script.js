// Client version of the tic-tac-toe.

// Global Variables
// ----------------------------------------------------------------------------

var grid;           // Grid of the game
var sign;           // Sign associated to the player ('x' or 'o')
var request;        // XmlHttpRequest object used to send requests
var my_go;          // 'true' if the player must play
var timer;          // Timer used to send the 'update' request
var msg;            // Element used to print messages
var table;          // Table used to display the grid
var restart_btn;    // Restart button
var title_sign;     // Image that represents the sign in the title
var grid_signs;     // Images that represent the Signs in the grid

// Functions
// ----------------------------------------------------------------------------

// Initializes the page.
function init()
{
    // Initializes some criteria of the request.
    request = new XMLHttpRequest();
    request.onload = request_callback;
    request.responseType = "text";
    
    // Gets the element used to print the message.
    msg = document.querySelector("p.message");

    // Gets the table used to display the grid.
    table = document.querySelector("table");

    // Gets the restart button.
    restart_btn = document.querySelector("button");

    // Gets the sign displayed in the title.
    title_sign = document.querySelector("h1 img");

    // Gets all the signs in the grid.
    grid_signs = document.querySelectorAll("img.sign");

    // Gets the sign of the player.
    sign = title_sign.alt;

    // Starts a new game.
    start();
}

// Starts a new game.
function start()
{
    // Initializes an empty grid.
    grid = [ "_", "_", "_", "_", "_", "_", "_", "_", "_" ];

    // Displays the sign in the title.
    title_sign.src = sign + ".png";
    title_sign.alt = sign;

    // The player with the crosses always starts.
    my_go = sign === 'x' ? true : false;

    // If it is not the player's go,
    // updates the grid every second.
    if (my_go == false)
        timer = window.setInterval(update, 1000);

    // Sets the opacity of table to the maximum.
    table.style.opacity = "100%";

    // Hides the restart button.
    restart_btn.style.display = "none";

    // Remove the message.
    msg.innerText = "";
}

// Restarts a new game.
function restart()
{
    // Swaps the signs: 'o' <-> 'x'
    sign = sign === 'x' ? 'o' : 'x';

    // Starts a new game.
    start();

    // Sends the 'restart' command to the server.
    request.open("get", "restart", true);
    request.send();
}

// Sends an 'update' command to the server.
function update()
{
    request.open("get", "update", true);
    request.send();
}

// Stops the game and prints a message.
function game_over(winner)
{
    // Stops the timer.
    window.clearInterval(timer);

    // Prevents the player to play.
    my_go = false;

    // Decreases the opacity of the table.
    table.style.opacity = "20%";

    restart_btn.style.display = "inline";

    // Prints a message.
    if (winner === 0)
        msg.innerText = "Draw!!!";
    else if (winner === sign)
        msg.innerText = "You Win!!!";
    else
        msg.innerText = "You Lose!!!";
}

// Updates the display of the grid.
function update_display()
{
    // For each cell of the grid,
    // sets the image associated to the sign.
    for (var i = 0; i < grid.length; i++)
    {
        switch (grid[i])
        {
            case "x":
                grid_signs[i].src = "x.png";
                grid_signs[i].alt = "x";
                break;

            case "o":
                grid_signs[i].src = "o.png";
                grid_signs[i].alt = "o";
                break;

            case "_":
                grid_signs[i].src = "blank.png";
                grid_signs[i].alt = " ";
                break;
        }
    }
}

// Sets the value of a cell by sending a message to the server.
function set_cell(i)
{
    // If it is the player's go and if the cell is empty.
    if (my_go == true && grid[i] == "_")
    {
        // Switches the flag of the player's go.
        my_go = false;

        // Updates the grid.
        grid[i] = sign;

        // Sends the 'set' command to the server.
        request.open("get", "set_" + sign + i, true);
        request.send();

        // Starts the timer to update the display every second.
        timer = window.setInterval(update, 1000);
    }
}

// Callback function of the request.
//
// This function is executed when the XMLHttpRequest transaction completes
// successfully.
//
// The response of the request is a string of characters that represents
// the grid.
function request_callback()
{
    // Saves the current grid.
    var old_grid = grid.join('');

    // Gets the new grid.
    var new_grid = request.responseText;

    // If the grid has changed,
    // it means that the the other player has played.
    if (old_grid != new_grid)
    {
        // Stops the timer.
        window.clearInterval(timer);

        // The player associated to this script must play.
        my_go = true;

        // Updates the global variable with the new grid.
        grid = Array.from(new_grid);
    }

    // Updates the display.
    update_display();

    // If there is a winner, stops the game.
    var winner = get_winner();
    if (winner !== -1)
        game_over(winner);
}

// Gets the winner when the game ends.
//
// Return Value
//
// -1 if the game is not over.
// 0 if the game is over with no winner.
// 'x' if the game is over and the player with crosses wins.
// 'o' if the game is over and the player with noughts wins.
function get_winner()
{
    var x = new Array(8);
    var o = new Array(8);

    // Tests the lines for 'x'.
    x[0] = grid[0] == "x" && grid[1] == "x" && grid[2] == "x";
    x[1] = grid[3] == "x" && grid[4] == "x" && grid[5] == "x";
    x[2] = grid[6] == "x" && grid[7] == "x" && grid[8] == "x";

    // Tests the columns for 'x'.
    x[3] = grid[0] == "x" && grid[3] == "x" && grid[6] == "x";
    x[4] = grid[1] == "x" && grid[4] == "x" && grid[7] == "x";
    x[5] = grid[2] == "x" && grid[5] == "x" && grid[8] == "x";

    // Tests the diagonals for 'x'.
    x[6] = grid[0] == "x" && grid[4] == "x" && grid[8] == "x";
    x[7] = grid[2] == "x" && grid[4] == "x" && grid[6] == "x";

    // Tests the lines for 'o'.
    o[0] = grid[0] == "o" && grid[1] == "o" && grid[2] == "o";
    o[1] = grid[3] == "o" && grid[4] == "o" && grid[5] == "o";
    o[2] = grid[6] == "o" && grid[7] == "o" && grid[8] == "o";

    // Tests the columns for 'o'.
    o[3] = grid[0] == "o" && grid[3] == "o" && grid[6] == "o";
    o[4] = grid[1] == "o" && grid[4] == "o" && grid[7] == "o";
    o[5] = grid[2] == "o" && grid[5] == "o" && grid[8] == "o";

    // Tests the diagonals for 'o'.
    o[6] = grid[0] == "o" && grid[4] == "o" && grid[8] == "o";
    o[7] = grid[2] == "o" && grid[4] == "o" && grid[6] == "o";

    // If any values of x[] is true, the player 1 wins.
    if (x.find(e => e))
        return 'x';

    // If any values of o[] is true, the player 2 wins.
    if (o.find(e => e))
        return 'o';

    // If at least one element of the grid is empty,
    // the game is not over.
    if (grid.find(e => e == "_"))
        return -1;

    // Otherwise, the game is over with no winner.
    return 0;
}
