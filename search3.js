
// _____________________________________________________________________
function handleSelectCheckbox(el) {
  var checked = el.checked;
  var wdName = el.parentNode.parentNode.getAttribute("wdName");
  selectedVariables[wdName] = checked;
  updateOrderRadioButtons();
}

// _____________________________________________________________________
function updateOrderRadioButtons() {
  // determine previously checked variable
  var previouslyChecked=false;
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
      document.getElementById(id).checked="checked";
      //$("#" + id).attr("checked", "checked");
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

// _______________________________________________________________________
function determineSettingString(selectedArray) {
  var res = "?s=";
  var radioType = document.getElementsByName("orderAsc");
  for (var i = 0; i < radioType.length; i++) {
    if (radioType[i].checked) {
      res = res + radioType[i].value;
      break;
    }
  }
  radioType = document.getElementsByName("orderType");
  for (var i = 0; i < radioType.length; i++) {
    if (radioType[i].checked) {
      res = res + radioType[i].value;
      break;
    }
  }
  radioType = document.getElementsByName("orderVar");
  var orderVariableName="none";
  for (var i = 0; i < radioType.length; i++) {
    if (radioType[i].checked) {
      orderVariableName = radioType[i].value;
      break;
    }
  }
  var variableFound = false;
  var varIdx;
  for (var i = 0; i < selectedArray.length; i++) {
    if (orderVariableName == selectedArray[i]) {
      varIdx = i;
      variableFound = true;
      break;
    }
  }
  if (variableFound) {
    res += varIdx;
  } else {
    res = "?s=ax0";  // means no ordering
  }
  
  return res;
}
