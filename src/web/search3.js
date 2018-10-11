// When selecting or unselecting one of the variables we have to change some internal data structures
// to also propagate the change in "selectedness" to the internal logic.
// this is done by this function. Argument el is the select checkbox whose value was changed
function handleSelectCheckbox(el) {
    var checked = el.checked;
    var wdName = el.parentNode.parentNode.getAttribute("wdName");
    selectedVariables[wdName] = checked;
    updateOrderRadioButtons();
}

// When our set of selected variables change (because of renaming or because of selecting or unselecting)
// We also have to change the radio buttons for the "order by" clauses.
// This is done by this function
function updateOrderRadioButtons() {
    // determine previously checked variable
    var previouslyChecked = false;
    children = $("#orderButtons :input");
    for (var i = 0; i < children.length; i++) {
        if (children[i].checked) {
            previouslyChecked = children[i].id;
            break;
        }
    }
    $("#orderButtons").empty();
    $("#orderButtons").append("Order by: ")

    $("#orderButtons").append("<input type=\"radio\" id=\"orderNone\" name=\"orderVar\" value=\"\" checked=\"checked\">");
    $("#orderButtons").append("<label for=\"orderNone\">none</label>");
    var varFound = false;
    var selectedVars = selectedVarsToArray();
    for (var i = 0; i < selectedVars.length; i++) {
        var variable = selectedVars[i];

        varFound = true;
        var id = "order" + variable;
        $("#orderButtons").append("<input type=\"radio\" id=\"" + id + "\" name=\"orderVar\" value=\"" + variable + "\">");
        $("#orderButtons").append("<label for=\"" + id + "\">" + variable + "</label>");
        if (id == previouslyChecked) {
            document.getElementById(id).checked = "checked";
        }
    }
    if (!varFound) {
        $("#orderButtons").empty();
        $("#orderButtonsType").css("display", "none");
        $("#orderButtonsAsc").css("display", "none");
    } else {
        $("#orderButtonsType").css("display", "block");
        $("#orderButtonsAsc").css("display", "block");

    }
}


