/*
 * Check if all triples are valid (either completely filled or empty)
 * Delete empty triples, create sparql query and execute it
 */
function prepareAndExecuteSparqlQuery() {
    triples = document.getElementById("triples").children;
    var triplesToBeDeleted = [];
    var invalidTripleFound = 0;
    var sparql = ""

    // for all triples check if they are either completely filled
    // or completely empty
    for (var i = 0; i < triples.length; i++) {
        var name = triples[i].getAttribute("id");
        if (!name.startsWith("triple")) {
            continue;
        }
        var id = parseInt(name.substring(6));
        var els = triples[i].children;
        var oneDefined = 0;
        var oneUndefined = 0;
        var usedVariables = {}
        triple_vals = []
        for (var k = 0; k < els.length; k++) {
            // skip elements that are not actually triple elements
            if (els[k].getAttribute("class").startsWith("deleteTr")) {
                continue;
            }
            // check if the triple element is filled
            var wdName = els[k].getAttribute("wdName");
            if (wdName) {
                oneDefined = 1;
                triple_vals.push(wdName);
                if (wdName.startsWith("?")) {
                    usedVariables[wdName] = true;
                }
            } else {
                oneUndefined = 1;
                els[k].style.backgroundColor = "FF0000";
            }
        }


        if (oneDefined == 0) {
            // this is an empty triple
            triplesToBeDeleted.push(id);
        } else if (oneDefined == 1 && oneUndefined == 1) {
            // this triple is partially filled and thus invalid
            invalidTripleFound = 1;
        } else {
            // this is a valid triple, add it to our sparql clause
            sparql = sparql + composeTriple(triple_vals[0], triple_vals[1], triple_vals[2]);
        }
    }
    // delete all the empty triples from the gui to clean up
    for (var j = 0; j < triplesToBeDeleted.length; j++) {
        removeTriple(triplesToBeDeleted[j]);
    }

    // complete the SPARQL query
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
    PREFIX schema: <http://schema.org/>
    PREFIX qlever: <http://qlever.informatik.uni-freiburg.de/>
    `;
    sparqlHead += " SELECT";
    var selectedVarFound = false;

    // get deterministic order
    var selectedVars = selectedVarsToArray();
    for (var i = 0; i < selectedVars.length; i++) {
        curVar = selectedVars[i];
        if (usedVariables[curVar] == true) {
            sparqlHead += " " + curVar;
            selectedVarFound = true;
        }
    }
    sparqlHead += " WHERE \{ ";
    sparql = sparqlHead + sparql;
    var orderSettings = determineSettingString(selectedVars);
    sparql += orderSettings[1];
    sparql += "\}";
    sparql += " LIMIT 100000";
    sparql += orderSettings[0];
    // clear for new errors etc
    $("#queryRes").empty();
    if (invalidTripleFound) {
        showErrorInResline("Incomplete Triples were found (see red markings", "queryRes");
    } else if (!selectedVarFound) {
        showErrorInResline("You have to select at least one variable which also occurs in triples", "queryRes");
    } else {

        executeSparqlQuery(sparql, selectedVars);
    }
}

/* Send the sparql query back to the server and trigger the result showing
 * should come from the query result
 */
function executeSparqlQuery(query, selectedArray) {
    console.log(query);
    var host = window.location.host + window.location.pathname;
    var url = "http://" + host + "?r=" + escape(query);
    $.getJSON(url, function (data) {
        showResults(data, "queryRes");
    });
}


// create a pair of [orderClause, additionalTriples] that have to be added
// to the SPARQL query because of the selected order settings.
function determineSettingString(selectedArray) {
    var orderClause = " ORDER BY";
    var additionalTriples = "";

    // determine whether the ordering is ascending or descending
    var radioType = document.getElementsByName("orderAsc");
    for (var i = 0; i < radioType.length; i++) {
        if (radioType[i].checked) {
            if (radioType[i].value == 'a') {
                orderClause += " ASC(";
            } else {
                orderClause += " DESC(";
            }

            break;
        }
    }

    // determine which sort type (value, numSitelinks or alphabetic label) was selected
    var sortType = "none";
    radioType = document.getElementsByName("orderType");
    for (var i = 0; i < radioType.length; i++) {
        if (radioType[i].checked) {
            sortType = radioType[i].value;
            break;
        }
    }

    // determine for which variable the ordering will be
    radioType = document.getElementsByName("orderVar");
    var orderVariableName = "none";
    var modifiedOrderVariableName = "none";
    for (var i = 0; i < radioType.length; i++) {
        if (radioType[i].checked) {
            // we will order by this variable
            orderVariableName = radioType[i].value;
            if (sortType == 's') {
                // sorting by number of sitelinks.
                var sitelinkVar = orderVariableName + "numLinks";
                var metaNodeVar = orderVariableName + "metaNode";
                additionalTriples = " " + metaNodeVar + " wikibase:sitelinks " + sitelinkVar + " . ";
                additionalTriples += " " + metaNodeVar + " schema:about " + orderVariableName + " . ";
                modifiedOrderVariableName = sitelinkVar;
            } else if (sortType == 'a') {
                // sorting by alphabetical value of labels
                sitelinkVar = orderVariableName + "label";
                additionalTriples = " " + orderVariableName + " @en@rdfs:label " + sitelinkVar + " . ";
                modifiedOrderVariableName = sitelinkVar;
            } else {
                modifiedOrderVariableName = orderVariableName;
            }
            orderClause += modifiedOrderVariableName + ') ';
            break;
        }
    }

    // check if the variable we want to order by is selected.
    // If not: do not modify the query
    // also handles the case that we selected "none" as a order variable
    var variableFound = false;
    for (var i = 0; i < selectedArray.length; i++) {
        if (orderVariableName == selectedArray[i]) {
            variableFound = true;
            break;
        }
    }
    if (!variableFound) {
        orderClause = "";
        additionalTriples = "";
    }

    return [orderClause, additionalTriples];
}
