// COPYRIGHT 2016 University of Freiburg
// Author Johannes Kalmbach <johannes.kalmbach@gmail.com>


/* show the Wikidata entities in argument json in the <div> whose id is argument
 * basename
 */
function showEntitiesInResline(json, basename) {
    $("#" + basename).empty();
    var retJson = json["entities"];
    for (var i = 0; i < retJson.length; i++) {

        var cssClass = retJson[i]["type"] == "1" ? "resLinePredicate" : "resLineSubject";
        $("#" + basename).append("<div class =\"" + cssClass + "\" id=res" + basename + i + " >");
        showSingleEntity("res" + basename + i, retJson[i]);
    }
}

/* show the Result of a sparql query denoted by json within the tag with id
 * "basename". We also have to know the selectedVars of the Query (has to be
 * removed
 */
function showResults(json, basename) {
    $("#" + basename).empty();
    if (json["status"] != "OK") {
        $("#" + basename).text("QLever backend sent an error : " + json["exception"]);
        return;
    }
    // TODO: more elegant layout for this
    $("#" + basename).append("Query: ");
    $("#" + basename).append(escapeHtml(json["query"]));
    $("#" + basename).append("<br />Number of result triples: ");
    $("#" + basename).append(escapeHtml(json["resultsize"]));


    var tableId = "restable";
    $("#" + basename).append("<table id=\"" + tableId + "\"></table>");
    var retJson = json["res"];
    $("#" + tableId).append("<tr id=\"" + tableId + "h\"></tr>");
    var selectedVars = json["selected"]
    for (var i = 0; i < selectedVars.length; i++) {
        var rId = tableId + "h" + i
        $("#" + tableId + "h").append("<td id=\"" + rId + "\"></td>");
        $("#" + rId).text(selectedVars[i])
    }

    for (var i = 0; i < retJson.length; i++) {
        var rId = tableId + i
        $("#" + tableId).append("<tr id=\"" + rId + "\"></td>");
        for (var j = 0; j < retJson[i].length; j++) {
            var eId = rId + "c" + j;
            var el = retJson[i][j];
            var cssClass = "resLineSubject";
            $("#" + rId).append("<td id=\"" + eId + "\"></td>");
            showSingleEntity(eId, el);

        }
    }
}

/* show a query related error (error message as string) in the tag with id
 * basename
 */
function showErrorInResline(error, basename) {
    $("#" + basename).empty();
    var cssClass = "resLinePredicate"
    $("#" + basename).append("<div class =\"" + cssClass + "\" id=\"error" + basename + "\" >");
    $("#error" + basename).text(error);
}

/* add the css class "marked" to the target of the dragdrop event "event"*/
function markPossibleDragTarget(event) {
    $("#" + event.target.id).addClass("marked");
    var wdType = event.dataTransfer.getData("wdType");
    var targetType = $("#" + event.target.id).hasClass("property") ? 1 : 0;
    if (!isTypeMatch(event) && !targetAlreadyWrongType(event)) {
        $("#" + event.target.id).addClass("stripes");
    } else if (isTypeMatch(event) && targetAlreadyWrongType(event)) {
        $("#" + event.target.id).removeClass("stripes");
    }
}


/* undo effect of function markPossibleDragTarget */
function unmarkPossibleDragTarget(event) {
    $("#" + event.target.id).removeClass("marked");
    if (!isTypeMatch(event) && !targetAlreadyWrongType(event)) {
        $("#" + event.target.id).removeClass("stripes");
    } else if (isTypeMatch(event) && targetAlreadyWrongType(event)) {
        $("#" + event.target.id).addClass("stripes");
    }
}

// Returns true iff the target of argument event already had the wrong type
// (e.g. a P... Propertie as a subject or object
function targetAlreadyWrongType(event) {
    return $("#" + event.target.id).attr("wrongWdType") == 1;
}

// Returns true iff event wants to drop a Property to a Predicate field
// or a subject/Object (Q...) to a subject/object field
function isTypeMatch(event) {
    var wdType = event.dataTransfer.getData("wdType");
    // this means "variable" and those can currently be placed anywhere
    // without complaints
    if (wdType > 1) return true;
    var targetType = $("#" + event.target.id).hasClass("property") ? 1 : 0;
    return (targetType == wdType);
}

/* show the textinput field which allows renaming of the variable which is
 * stored in object "el". (these have class "variable" in the html)
 */
function showRenamingTexfield(el) {
    idLabel = el.id + "Label";
    idInput = el.id + "Input";
    var label = $("#" + idLabel);
    var input = $("#" + idInput);
    label.css('display', 'none');
    input.css('display', 'inline-block');
    input.val(label.text());
    input.focus();
}

/* Everything we have to do after a variable rename is finished. el is the input
 * tag used for variable renaming
 */
function renameVariableFinished(el) {
    var id = el.id.slice(0, 2);
    var idInput = el.id;
    var idLabel = id + "Label";
    var label = $("#" + idLabel);
    var input = $("#" + idInput);
    var outer = $("#" + id);
    var oldVal = outer.attr("wdName");
    label.css('display', 'inline-block');
    input.css('display', 'none');
    label.text(input.val());
    outer.attr("readableName", input.val());
    outer.attr("wdName", input.val());
    updateVariableNameInternally(oldVal, input.val());
    updateOrderRadioButtons();

}

/* ensures that only valid variable names (start with ? and then alphanumeric)
 * are entered into textinput field "el" */
function restrictVariableRename(el) {
    var rem = ""
    if (el.value.slice(0, 1) != "?") {
        rem = el.value;
    } else {
        rem = el.value.substring(1);
    }

    el.value = "?" + rem.replace(/[^a-z0-9]/g, "");
}

/* makes sure that the input field corresponding to keystroke event "ev" also
 * loses focus on Enter and Escape keys */
function handleEnterEscape(ev) {
    if (ev.keyCode == 13 || ev.keyCode == 27) {
        ev.target.blur();
    }
}

/* register that the variable with name "wdName" is now used in the triple
 * subject, property or object with id "targetId". Necessary for correctly
 * renaming of variables which are already in use */
function addVariableUsage(wdName, targetId) {
    if (!(wdName in variableUsages)) {
        variableUsages[wdName] = [];
    }
    variableUsages[wdName].push(targetId);
}

/* register that variable with name "wdName" is no longer used in tripe sub,
 * pred or obj with id "targetId". Necessary for correct renaming of variables
 * which are already in use */
function removeVariableUsage(wdName, targetId) {
    if (wdName in variableUsages) {
        var index = variableUsages[wdName].indexOf(targetId);
        if (index !== -1) variableUsages[wdName].splice(index, 1);
    }
}

/* make sure that after a variable renaming all the occurences of this variable
 * are also being renamed correctly
 */
function updateVariableNameInternally(oldName, newName) {
    if (oldName in variableUsages) {
        for (var idx = 0; idx < variableUsages[oldName].length; idx++) {
            var id = variableUsages[oldName][idx]
            var el = $("#" + id);
            el.text(newName);
            el.attr("wdName", newName);
        }
        variableUsages[newName] = variableUsages[oldName];
        delete variableUsages[oldName];
    }
    if (oldName in selectedVariables && selectedVariables[oldName]) {
        selectedVariables[oldName] = false;
        selectedVariables[newName] = true;
    }
}

// ____________________________________________________________________
function showSingleEntity(parentId, entity) {
    var newId = parentId + "inner";
    // make entity draggable
    entity["wdName"] = escapeHtml(entity["wdName"])
    entity["name"] = escapeHtml(entity["name"])
    entity["description"] = escapeHtml(entity["description"])
    var cssClass = entity["type"] == "1" ? "resLinePredicate" : "resLineSubject";
    $("#" + parentId).append("<div id=\"" + newId + "\" " +
        "class=\"" + cssClass + "\" " +
        " draggable=\"true\" ondragstart=\"drag(event)\"" +
        "ondragend=\"endDrag(event)\"" +
        "wdName=\"" + entity["wdName"] + "\" readableName=\"" + entity["name"] + "\"" +
        "wdType=\"" + entity["type"] + "\"" +
        "description=\"" + entity["description"] + "\">");
    parentId = newId
    var wdId = parentId + "wd";
    var nameId = parentId + "nm";
    var descId = parentId + "desc";
    $("#" + parentId).append("<span class=\"entityName\" id=\"" + nameId + "\"></span>");
    $("#" + parentId).append("<span class=\"wdName\" id=\"" + wdId + "\"></span>");
    $("#" + parentId).append("<div class=\"entityDescription\" id=\"" + descId + "\"></div>");


    // we have already escaped the names, so we can set the html
    $("#" + wdId).html(entity["wdName"]);
    $("#" + nameId).html(entity["name"]);
    $("#" + descId).html(entity["description"]);

}

// ______________________________________________________________________
function escapeHtml(input) {
    return input.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

// _________________________________________________________________________
function composeTriple(sub, pred, ob) {
    //TODO only if all is clicked etc.
    /*
    if (!pred.startsWith("?")) {
      pred = "<A" + pred.substring(1)
    }
    */
    // var ob_rep = "?" + ob;
    var actual_triple = addPrefix(sub, "wd") + " " + addPrefix(pred, "wdt") + " " + addPrefix(ob, "wd") + " .\n";
    return actual_triple;
}

// __________________________________________________________________
function addPrefix(entity, prefix) {
    if (entity.startsWith("?")) {
        return entity;
    }
    withoutBrackets = entity.substring(1, entity.length - 1);
    return prefix + ":" + withoutBrackets;
}

// ______________________________________________________________
function selectedVarsToArray() {
    arr = []
    for (var variable in selectedVariables) {
        if (selectedVariables.hasOwnProperty(variable) && selectedVariables[variable]) {
            arr.push(variable);
        }
    }
    return arr;
}

