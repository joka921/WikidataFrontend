// COPYRIGHT 2016 University of Freiburg
// Author Johannes Kalmbach <johannes.kalmbach@gmail.com>

/* A helper function which shows the actual source code of the current html
 * page including dynamic changes made by javascript etc 
 */
function viewSource(){
    var source = "<html>";
    source += document.getElementsByTagName('html')[0].innerHTML;
    source += "</html>";
    source = source.replace(/</g, "&lt;").replace(/>/g, "&gt;");
    source = "<pre>"+source+"</pre>";
    sourceWindow = window.open('','Source of page','height=800,width=800,scrollbars=1,resizable=1');
    sourceWindow.document.write(source);
    sourceWindow.document.close(); 
    if(window.focus) sourceWindow.focus();
}  

/* TODO: this function has a really bad name and does too many things at once
 * Check if all triples are valid (either completely filled or empty)
 * Delete empty triples, create sparql query and execute it
 * (too much for one function, as said)
 */
function removeEmptyTriples() {
   triples = document.getElementById("triples").children;
   var triplesToBeDeleted = [];
   var invalidTripleFound = 0;
   var sparql = ""
  for (var i = 0; i < triples.length; i++) {
    var name = triples[i].getAttribute("id");
    if (!name.startsWith("triple")) {
      continue;
    }
    var id = parseInt(name.substring(6));
    var els = triples[i].children;
    var oneDefined = 0;
    var oneUndefined = 0;
    var usedVariables= {}
    triple_vals = []
    for (var k = 0; k < els.length; k++) {
      if (els[k].getAttribute("class").startsWith("deleteTr")) {
        continue;
      }
      console.log(els[k]);
      var wdName = els[k].getAttribute("wdName");
      if (wdName) {
        oneDefined = 1;
	triple_vals.push(wdName);
        if (wdName.startsWith("?")) {
          usedVariables[wdName] = true;
        }
      } else {
        oneUndefined = 1;
        els[k].style.backgroundColor="FF0000";
        console.log(els[k])
      }
    }
    if (oneDefined == 0) {
      triplesToBeDeleted.push(id);
    } else if (oneDefined == 1 && oneUndefined == 1) {
      // mark incomplete already done above, TODO: remember original color
      invalidTripleFound = 1;
    } else {
      sparql = sparql + composeTriple(triple_vals[0], triple_vals[1], triple_vals[2]);
    }
  }

  for (var j = 0; j < triplesToBeDeleted.length; j++) {
    removeTriple(triplesToBeDeleted[j]);
  }
  sparqlHead = `
    PREFIX wd: <http://www.wikidata.org/entity/> 
    PREFIX wds: <http://www.wikidata.org/entity/statement/>
    PREFIX wdv: <http://www.wikidata.org/value/>
    PREFIX wdt: <http://www.wikidata.org/prop/direct/>
    PREFIX wikibase: <http://wikiba.se/ontology#>
    PREFIX p: <http://www.wikidata.org/prop/>
    PREFIX ps: <http://www.wikidata.org/prop/statement/>
    PREFIX pq: <http://www.wikidata.org/prop/qualifier/>
    PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
    PREFIX bd: <http://www.bigdata.com/rdf#>
    `
  sparqlHead += " SELECT";
  var selectedVarFound = false;
  /*
  checkboxes = $('input[id^="selectedX"]');
  for (var c in checkboxes) {
    c = checkboxes[c];
    // javascript thing: with c.checked we verify that we indeed have found a
    // checkbox and avoid type errors
    var currentVariableName = c.checked ? c.parentNode.parentNode.getAttribute("wdName") : "";

    if (c.checked == true &&usedVariables[currentVariableName] == true) {
      sparqlHead += " "  + currentVariableName;
      selectedVarFound = true;

    }
  }
  */

  // get deterministic order
  var selectedVars = selectedVarsToArray();
  for (var i = 0; i < selectedVars.length; i++) {
    curVar = selectedVars[i];
    if (usedVariables[curVar] == true) {
      sparqlHead += " "  + curVar;
      selectedVarFound = true;
    }
  }
  sparqlHead += " WHERE \{ ";
  sparql = sparqlHead + sparql;
  sparql += "\}";
  // clear for new errors etc
  $("#queryRes").empty();
  if (invalidTripleFound) {
    showErrorInResline("Incomplete Triples were found (see red markings", "queryRes");
  } else if (!selectedVarFound) {
    showErrorInResline("You have to select at least one variable which also occurs in triples", "queryRes");
  } else {
    // TODO: also filter empty queries!!!
    sparql += " LIMIT 100000"
    executeSparqlQuery(sparql, selectedVars);
  }
}

/* Send the sparql query back to the server and trigger the result showing
 * TODO: currently we have to keep track of the selected variables, but those
 * should come from the query result (TODO in backend)
 */
function executeSparqlQuery(query, selectedArray) {
    console.log(query);
    var host = window.location.host + window.location.pathname;
    // TODO: not- hardcoded port (what is the easiest way ??)
    var orderSettings = determineSettingString(selectedArray);
    query += orderSettings[1];
    var url = "http://" + host + orderSettings[0] +  "?r=" + escape(query);
    //url=escape(url);
    console.log(url);
    $.getJSON(url, function(data) {
      showResults(data, "queryRes");
    });
}

/* show the Wikidata entities in argument json in the <div> whose id is argument
 * basename
 * TODO: name also is not optimal and code is not nicely looking
 */
function showEntitiesInResline(json, basename) {
  $("#" + basename).empty();
  var retJson = json["entities"];
  for (var i = 0; i < retJson.length; i++) {

    var cssClass = retJson[i]["type"] == "1" ? "resLinePredicate" : "resLineSubject";
    $("#" + basename).append("<div class =\"" + cssClass +"\" id=res" + basename + i + " >");
    showSingleEntity("res" + basename + i, retJson[i]);
  }
}

/* show the Result of a sparql query denoted by json within the tag with id
 * "basename". We also have to know the selectedVars of the Query (has to be
 * removed, todo is in other function already)
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
  $("#" + tableId).append("<tr id=\"" + tableId +"h\"></tr>");
  var selectedVars = json["selected"]
  for (var i = 0; i < selectedVars.length; i++) {
    var rId = tableId + "h" + i
    $("#" + tableId + "h").append("<td id=\"" + rId + "\"></td>");
    $("#" + rId).text(selectedVars[i])
  }

  for (var i = 0; i < retJson.length; i++) {
    var rId = tableId + i
    $("#" + tableId).append("<tr id=\"" + rId +"\"></td>");
    for (var j = 0; j < retJson[i].length; j++) {
      var eId = rId + "c" + j;
      var el = retJson[i][j];


      //TODO include this again
      //var cssClass = el["type"] == "1" ? "resLinePredicate" : "resLineSubject";
      var cssClass = "resLineSubject";
      $("#" + rId).append("<td id=\"" + eId + "\"></td>");
      showSingleEntity(eId, el);
      //var text = el["wdName"] + "\n" + el["name"] + "\n" + el["description"];
        //console.log(text);
      //$("#" + eId).text(text);
      //$("#wdDesc" + basename + i).text(retJson[i]["desc"]);
     }
  }
}

/* show a query related error (error message as string) in the tag with id
 * basename
 */
function showErrorInResline(error, basename) {
  $("#" + basename).empty();
  var cssClass = "resLinePredicate"
  $("#" + basename).append("<div class =\"" + cssClass +"\" id=\"error" + basename +"\" >");
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

function targetAlreadyWrongType(event) {
  return $("#" + event.target.id).attr("wrongWdType") == 1;
}

function isTypeMatch(event) {
  var wdType = event.dataTransfer.getData("wdType");
  // this means "variable" and those can currently be placed anywhere
  // without complaints
  if (wdType > 1) return true;
  var targetType = $("#" + event.target.id).hasClass("property") ? 1 : 0;
  return (targetType == wdType);
}

/* TODO: bad function name, this does not actually rename
 * show the textinput field which allows renaming of the variable which is
 * stored in object "el". (these have class "variable" in the html)
 */
function renameVariable(el) {
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
  var newId = parentId +"inner";
  // make entity draggable
  entity["wdName"] = escapeHtml(entity["wdName"])
  entity["name"] = escapeHtml(entity["name"])
  entity["description"] = escapeHtml(entity["description"])
  var cssClass = entity["type"] == "1" ? "resLinePredicate" : "resLineSubject";
  $("#" + parentId).append("<div id=\"" + newId + "\" " +
                               "class=\"" + cssClass + "\" " +
                               " draggable=\"true\" ondragstart=\"drag(event)\"" +
                               "ondragend=\"endDrag(event)\""  +
                               "wdName=\"" + entity["wdName"] + "\" readableName=\"" + entity["name"] + "\"" +
                               "wdType=\"" + entity["type"] + "\""  +
                               "description=\"" + entity["description"] +"\">");
  parentId = newId
  var wdId = parentId + "wd";
  var nameId = parentId + "nm";
  var descId = parentId + "desc";
  $("#" + parentId).append("<span class=\"entityName\" id=\"" + nameId +"\"></span>");
  $("#" + parentId).append("<span class=\"wdName\" id=\"" + wdId +"\"></span>");
  $("#" + parentId).append("<div class=\"entityDescription\" id=\"" + descId +"\"></div>");
  
  
  // we have already escaped the names, so we can set the html
  $("#" + wdId).html(entity["wdName"]);
  $("#" + nameId).html(entity["name"]);
  $("#" + descId).html(entity["description"]);
  
}

// ______________________________________________________________________
function escapeHtml(input) {
  return input.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
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

