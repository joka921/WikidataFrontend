// COPYRIGHT 2016 University of Freiburg
// Author Johannes Kalmbach <johannes.kalmbach@gmail.com>


// Global variables

// the number of triples currently displayed in the gui
var numTriples = 1;

// the unique index the next triple which will be added to gui will get.
// Can differ from numTriples after triples have been deleted
var nextIndexTriple = 1;

// stores information which variables are used in which triples
// e.g. "object1" in variableUsages["?x0"] means that the object field of triple
// 1 currently holds the variable value of ?x0. Necessary for correct renaming
// of variables which are already in use
var variableUsages = {};

var selectedVariables = {};




// TODO: maybe register all the ondragstart etc functions here to clean up the
// html
$(document).ready(function() {
  $("#query").keyup(getEntitySearchResults);
  $("#searchmodeButtons").change(getEntitySearchResults);
})

/* get the wikidata entities corresponding to the value in textfield #query from
 * server and show them in the #searchRes field
 */
function getEntitySearchResults() {
    var query = $("#query").val();
    var host = window.location.host;
    var port = window.location.port;
    host = host + window.location.pathname;
    var url = "http://" + host  + "?t=" + "obj" + "?q=" + query;
    console.log("URL: " + url);
    $.getJSON(url, function(data) {
      if (data["entities"]) {
        showEntitiesInResline(data, "searchRes");
      }
    });
    url = "http://" + host + "?t=" + "prd" + "?q=" + query;
    console.log("URL: " + url);
    $.getJSON(url, function(data) {
      if (data["entities"]) {
        showEntitiesInResline(data, "searchResPred");
      }
    });
  }


/* add a triple to the gui which can be filled via drag and drop */
function addTriple() {
  var i = nextIndexTriple;
  var end = "\" ondrop=\"drop(event)\" ondragover=\"allowDrop(event)\"" +
            "ondragenter=\"markPossibleDragTarget(event)\"" +
            " draggable=\"true\" ondragstart=\"drag(event)\"" +
            "ondragend=\"endDrag(event)\""  +
            "onclick=\"showDetails(this)\"" +
            "ondragleave=\"unmarkPossibleDragTarget(event)\" > </div>"
  $("#triples").append("<div class=\"triple\" id=\"triple" + nextIndexTriple +"\" >" +
        "<div class=\"subject\" id=\"subject" + i + end +
        "<div class=\"property\" id=\"property" + i + end +
        "<div class=\"object\" id=\"object" + i + end +
        "<button class=\"deleteTripleButton\" onclick=\"removeTriple(" + nextIndexTriple + ")\"> - </button>" +
      "</div>"
      );
  numTriples = numTriples + 1;
  nextIndexTriple = nextIndexTriple + 1;
}

/* remove the Triple with index "idx" if this was the last triple, make sure
 * that there remains at least on triple in the gui*/
function removeTriple(idx) {
 $("#triple" + idx).remove();
 numTriples -= 1;
 if (numTriples <= 0) {
   addTriple();
 }
}

// __________________________________________________________________________
function allowDrop(ev) {
    ev.preventDefault();
    markPossibleDragTarget(ev);
}

/* handle the start of a drag&drop operation*/
function drag(ev) {
    // custom drag drop image which has size of a triple element and shows only
    // the name of the dragged entity
    // for details see https://kryogenix.org/code/browser/custom-drag-image.html
    var crt = ev.target.cloneNode(true);
    crt.id = crt.id + "dummy"

    var rect = $(".subject")[0];
    crt.style.width = rect.clientWidth;
    crt.style.height = rect.clientHeight;
    crt.style.position = "absolute"; crt.style.top = "0px"; crt.style.right = "0px";
    crt.style.zIndex = "-2";
    crt.innerText=ev.target.getAttribute("readableName");
    document.body.appendChild(crt);
    ev.dataTransfer.setDragImage(crt, rect.clientWidth/2, rect.clientHeight /2);

    // mark the origin of the drag
    //markPossibleDragTarget(ev);

    // set all the data of the entity to be transmitted
    ev.dataTransfer.setData("text", ev.target.getAttribute("readableName"));
    ev.dataTransfer.setData("dummyId", crt.id);
    ev.dataTransfer.setData("wdName", ev.target.getAttribute("wdName"));
    ev.dataTransfer.setData("description", ev.target.getAttribute("description"));
    ev.dataTransfer.setData("wdType", ev.target.getAttribute("wdType"));

}

/* handle the end of an entity or variable drag&drop operation*/
function drop(ev) {
    ev.preventDefault();
    // no need to keep the marking after finishing of op.
    unmarkPossibleDragTarget(ev);

    var data = ev.dataTransfer.getData("text");
    var wdName = ev.dataTransfer.getData("wdName");
    var desc = ev.dataTransfer.getData("description");
    var wdType = ev.dataTransfer.getData("wdType");
    var wdNameOld = ev.target.getAttribute("wdName");

    // if the target used to hold a variable, unregister this
    if (wdNameOld && wdNameOld.startsWith("?")) {
      removeVariableUsage(wdNameOld, ev.target.id)
    }
    ev.target.setAttribute("wdName", wdName);
    ev.target.setAttribute("description", desc);
    ev.target.setAttribute("wdType", wdType);
    ev.target.setAttribute("readableName", data);
    ev.target.innerHTML = data;

    if (!isTypeMatch(ev)) {
      $("#" + ev.target.id).addClass("stripes");
      $("#" + ev.target.id).attr("wrongWdType", 1);
    } else {
      $("#" + ev.target.id).removeClass("stripes");
      $("#" + ev.target.id).attr("wrongWdType", 0);
    }

    // element which was only there for the drag image
    removeDummyElement(ev);
    // keep track of variableUsages in case of renamings
    if (wdName.slice(0,1) == "?") {
      addVariableUsage(wdName, ev.target.id)
    }


}

// handle unfinished drag&drop operations
function endDrag(ev) {
  unmarkPossibleDragTarget(ev);
  removeDummyElement(ev);
}

// remove the element which was used for the drag&drop image
function removeDummyElement(ev) {
  $("#" + ev.dataTransfer.getData("dummyId")).remove();
}

/* each triple field shows only the name of entities but also saves descriptions
 * etc. This function shows the details of a given triple element "el" in the
 * tag with id "detailRes"*/
function showDetails(el) {
  if (el.innerText=="") {
    return;
  }

  $("#detailRes").empty();
  var entity = {}
  entity["type"] = el.getAttribute("wdType");
  entity["wdName"] = el.getAttribute("wdName");
  entity["name"] = el.innerText;
  entity["description"] = el.getAttribute("description");
  showSingleEntity("detailRes", entity);

}

