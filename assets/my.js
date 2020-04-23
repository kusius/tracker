import { MDCMenu } from '@material/menu';
import { MDCSnackbar } from '@material/snackbar';




const minutes_treshold = 60;
const menu = new MDCMenu(document.querySelector('.mdc-menu'));
const snackbar = new MDCSnackbar(document.querySelector('.mdc-snackbar'));
var last_row_menu;

// Register this callback function for our C++ update handler to call when 
// we have some new items to show the user
window.register(Notify);

export function Notify(item_list) {
    if (item_list.length == 0)
        console.log("Notify called with array of length: 0");
    else {
        var ulist = document.getElementById('notifylist');
        // Get the list item template and fill it
        var template = document.querySelector('#notifyitem');
        for (var i = 0, item; item = item_list[i]; i++) {
            if (item.mins_elapsed <= minutes_treshold) {
                var textnode = document.createTextNode(item.price + ' - ' + item.mins_elapsed + ' minutes ago');
                var clone = template.content.cloneNode(true);
                clone.getElementById('itemname').innerText = item.name;
                clone.getElementById('itemmeta').appendChild(textnode);
                clone.getElementById('itemlocation').innerText = item.location + " - " + item.trader;
                // Material styling
                //Append the new list item to the list
                ulist.appendChild(clone);
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


export function menuhandle(event) {
    if (event.which == 3) {
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
