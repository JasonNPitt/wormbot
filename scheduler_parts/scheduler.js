var open_menu = false;
var setOfOldTask =  new Set();
function changeBrightness() {
    if (!open_menu) {
        darken();
        open_menu = true;
    } else {
        brighten();
        open_menu = false;
    }
}

function brighten() {
    document.body.style.backgroundColor = "white";

    document.getElementById("main").style.filter = "brightness(100%)";
    //document.getElementById("open-sidebar").style.filter = "brightness(100%)";
}

function darken() {
    document.body.style.backgroundColor = "rgba(0,0,0,0.4)";

    document.getElementById("main").style.filter = "brightness(50%)";
    //document.getElementById("open-sidebar").style.filter = "brightness(50%)";
}

function controlSideBar() {
    let tempEl = document.createElement('div');
    tempEl.innerHTML = '&#60;';
    let currentState = document.getElementById("btn-sidebar");
    if (currentState.innerHTML === tempEl.innerHTML) {
        currentState.innerHTML = "&#62;";
        document.getElementById("SideBar").style.width = "0";
        document.getElementById("main").style.marginLeft = "0";
        brighten();
    } else {
        currentState.innerHTML = "&#60;";
        document.getElementById("SideBar").style.width = "250px";
        document.getElementById("main").style.marginLeft = "250px";
        if (menu_state === true) {
            darken();
        }
    }
}

function assignDefaultSelectOption() {
    var rows = document.querySelectorAll("[name^=row]");
    var wells = document.querySelectorAll("[name^=well]");
    // iterate throw twelve wells and assign default value accordingly
    var r_list = ["A", "B", "C"];
    var w_list = ["1", "2", "3", "4"];
    for (var i = 0; i < 12; i++) {
         var r = i / 4;
         var w = i % 4;
         rows[i].selectedIndex = r;
         wells[i].selectedIndex = w;
    }
}

// relocate the list
function movePlatesDisplay() {
    document.getElementById("open-slots").appendChild(document.getElementById("open-plates-display"));
}



var well2Num = new Map([
  ['A1', 0],
  ['A2', 1],
  ['A3', 2],
  ['A4', 3],
  ['B1', 4],
  ['B2', 5],
  ['B3', 6],
  ['B4', 7],
  ['C1', 8],
  ['C2', 9],
  ['C3', 10],
  ['C4', 11],
])


function fillSelectedPlateWithOldData(plateNum) {
    // plate# :task[1], well#: task[2], invextigator: task[3], title 9, description 10
        // strain 11, startingN 12; startingAge 13;

    if (plateNum == undefined) {
        plateNum = 1;
    }
    setOfOldTask.clear();
    var allSlots = document.querySelectorAll(".plate-info");
    for (var i = 0; i < allSlots.length; i++) {
        var temp = allSlots[i].querySelectorAll("input");
        for (var j = 2; j < temp.length; j++) {
            
            temp[j].value = '';
            
            //temp[j].value = "";

        }
        allSlots[i].style.filter = '';
        temp[temp.length - 1].checked = true;
        // reset access right
        var tempList = allSlots[i].childNodes;
        for (var j = 0; j < tempList.length; j++) {
            tempList[j].readOnly = false;
            // console.log(tempList[j].readOnly);
        }
    }

    var value = document.getElementById("select-plate").value;
    var taskTable = document.getElementById("task-table");
    var allTaskList = taskTable.rows;
    for (var i = 1; i < allTaskList.length; i++) {
        var task = allTaskList[i].cells;
        var well = task[2].innerHTML;
        var index = well2Num.get(well);
        if (task[1].innerHTML == plateNum) {
            // console.log(well + " " + num);
            var slot = allSlots[index].querySelectorAll("input");
            setOfOldTask.add(index);
            // manually assign to all
            slot[2].value = task[3].innerHTML;
            slot[3].value = task[8].innerHTML;
            slot[4].value = task[9].innerHTML;
            slot[5].value = task[10].innerHTML;
            slot[6].value = task[11].innerHTML;
            slot[7].value = task[12].innerHTML;
            slot[8].value = task[13].innerHTML;
            
                //console.log(task[10].disabled);
            
            slot[slot.length - 1].checked = false;
            allSlots[index].style.filter = "blur(2px)";

            // set readOnly
            var tempList = allSlots[index].childNodes;
            for (var j = 0; j < tempList.length; j++) {
                tempList[j].readOnly = true;
            }
        } 

    }
}

// confirm to overwirte
function confirmPlateUpdate() {
    return confirm("Selected slot contains a previous task. Do you still want to proceed?");
}

function confirmEdit(event) {
    var allSlots = document.querySelectorAll(".plate-info");
    var target = allSlots[event.currentTarget.index];
    var tempList = target.childNodes;
    if (setOfOldTask.has(event.currentTarget.index)) {
        if (confirmPlateUpdate()) {
            tempList[tempList.length - 1].checked = true;
            setOfOldTask.delete(event.currentTarget.index);
            for (var j = 0; j < tempList.length; j++) {
                tempList[j].readOnly = false;
            }
        }
    }
}

function blurSlot(event) {
    if (setOfOldTask.has(event.currentTarget.index)) {
        event.target.style.filter = 'blur(2px)';
    }
}

function unblurSlot(event) {
    event.target.style.filter = '';
}


function addAllSlotListeners() {
    var allSlots = document.querySelectorAll(".plate-info");
    for (var i = 0; i < allSlots.length; i++) {
        allSlots[i].index = i;
        allSlots[i].addEventListener("mouseover", unblurSlot, false);
        allSlots[i].addEventListener("mouseleave", blurSlot, false);
        allSlots[i].addEventListener("click", confirmEdit, false);
    }
}


/*
user click on the slot
if lock {
    tf = confirmPlateUpdate();
    if not confirmed, return;
    else if confimred, lock = false;
    adjust bluring, 
}
*/

// function makeReadOnlySlots() {
//     // traverse elements in the list, if read only, then make it read only
//     for () {

//     }
// }

// function onEditSlot() {
//     var lock = true; // change it to a value from a list later
//     if (lock) {
//         let confirmed = confirmPlateUpdate();
//         if (!confirmed) {
//             return;
//         } else {
//             // make readonly false and propogate
//         }
//     }
// }

