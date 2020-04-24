import { MDCMenu } from '@material/menu';
import { MDCSnackbar } from '@material/snackbar';


/*User-configured variables*/
export var update_interval_ms = 1000 * 60; //(ms) how frequently do we poll TTC for deals
export var minutes_treshold = 2 * 60; //(minutes) how many minutes until deal is considered 'old'


/*Module state variables*/
const menu = new MDCMenu(document.querySelector('.mdc-menu'));
const snackbar = new MDCSnackbar(document.querySelector('.mdc-snackbar'));

var last_row_menu = -1;
var intervalID = -1;
var notification_map = new Map();

window.onload = function () {
    // code goes here
    intervalID = setTimeout("window.GlobalFuncs.DoFindDeals();", update_interval_ms);
};

export function DoFindDeals() {
    console.log('dofinddeals');
    // C++ function
    window.ttc_find_deals(Notify);
    intervalID = setTimeout("window.GlobalFuncs.DoFindDeals();", update_interval_ms);
}

export function Notify(item_list) {
    if (item_list.length == 0)
        console.log("Notify called with array of length: 0");
    else {
        var ulist = document.getElementById('notifylist');
        // Get the list item template and fill it
        var template = document.querySelector('#notifyitem');
        for (var i = 0, item; item = item_list[i]; i++) {
            if (notification_map.has(item.trade_id)) {
                //TODO(George): update the time elapsed on already shown notification
            }
            else if (item.mins_elapsed <= minutes_treshold) {
                //New notification

                var textnode = document.createTextNode(item.price + ' - ' + item.mins_elapsed + ' minutes ago');
                var clone = template.content.cloneNode(true);
                clone.getElementById('itemname').innerText = item.name;
                clone.getElementById('itemmeta').appendChild(textnode);
                clone.getElementById('itemlocation').innerText = item.location + " - " + item.trader;

                ulist.appendChild(clone);
                notification_map.set(item.trade_id, item.name);
            }
        }
    }
}

export function HandleSearch() {
    var item_name = document.getElementById('item-text-box').value;
    // C++ CEF implementation
    window.ttc_price_check(item_name, UpdateWatchList);
    return true;
}

export function MenuHandle(event) {
    // TODO(George): '3' is going to be left click if user has left-handed mouse....
    if (event.which == 3)/*Right click*/ {
        var parent = event.currentTarget;
        //open menu in the cursor's current position
        menu.setAbsolutePosition(event.clientX, event.clientY);
        menu.open = !menu.open;
        if (menu.open)
            last_row_menu = parent;
    }
}

export function RemoveItem(event) {
    var item_name = last_row_menu.children[0].children[1].innerText;
    last_row_menu.remove();
    // C++ CEF implementation
    window.remove_watched_item(item_name);
}

export function UpdateWatchList(item) {
    if (item == undefined) {
        //Item not found
        snackbar.labelText = "Item could not be found!";
        snackbar.open();
        return;
    }
    else {
        if (!item.exists) {
            // This is the watclist table
            var tbody = document.getElementById('watchlist');
            // Get the row template and fill it
            var template = document.querySelector('#itemrow');
            var clone = template.content.cloneNode(true);
            var td = clone.querySelectorAll("td");
            td[0].querySelector('div').textContent = item.name;
            td[0].querySelector('img').src = item.img_src;
            td[1].textContent = item.min_suggest;
            td[2].textContent = item.max_suggest;
            //Append the new row to the table
            tbody.appendChild(clone);
        }
        else if (item.exists) {
            // Find item on watchlist and update its fields
            var tbody = document.getElementById('watchlist');
            for (var i = 0, row; row = tbody.rows[i]; i++) {
                if (row.cells[0].querySelector('div').textContent == item.name) {
                    // Found named item. Update prices
                    row.cells[1].textContent = item.min_suggest;
                    row.cells[2].textContent = item.max_suggest;
                }
            }
            snackbar.labelText = "Updated " + item.name + ". Already in watch list";
            snackbar.open();
            return;
        }

    }

}
