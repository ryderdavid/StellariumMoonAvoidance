#ifndef MOONAVOIDANCEDIALOG_HPP
#define MOONAVOIDANCEDIALOG_HPP

#include "StelDialog.hpp"
#include <QListWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QLabel>
#include <QCheckBox>
#include "MoonAvoidanceConfig.hpp"

class TitleBar;
class MoonAvoidance;

class MoonAvoidanceDialog : public StelDialog
{
	Q_OBJECT

public:
	MoonAvoidanceDialog();
	~MoonAvoidanceDialog() override;
	
	void setFilters(const QList<FilterConfig>& filters);
	QList<FilterConfig> getFilters() const;
	bool wasAccepted() const { return accepted; }
	void resetAccepted() { accepted = false; }

protected:
	void createDialogContent() override;

public slots:
	void retranslate() override;

private slots:
	void onFilterSelectionChanged(const QString& filterName);
	void addFilter();
	void removeFilter();
	void updateSeparation(double value);
	void updateWidth(double value);
	void updateRelaxation(double value);
	void updateMinAlt(double value);
	void updateMaxAlt(double value);
	void updateColor();
	bool validateInput();

private:
	void updateFormFields();
	void enableFormFields(bool enabled);
	void updateCurrentSeparation(); // Calculate and display current separation
	
	// Left panel: Filter list
	QListWidget* filterListWidget;
	
	// Right panel: Form fields
	QFormLayout* formLayout;
	QDoubleSpinBox* separationSpinBox;
	QDoubleSpinBox* widthSpinBox;
	QDoubleSpinBox* relaxationSpinBox;
	QDoubleSpinBox* minAltSpinBox;
	QDoubleSpinBox* maxAltSpinBox;
	QPushButton* colorButton;
	QLabel* colorLabel;
	QLabel* moonAgeLabel; // Read-only display of moon age (days since new moon)
	QLabel* currentSeparationLabel; // Read-only display of calculated current separation
	
	// Buttons
	QPushButton* addButton;
	QPushButton* removeButton;
	QPushButton* okButton;
	QPushButton* cancelButton;
	
	QList<FilterConfig> currentFilters;
	QList<FilterConfig> pendingFilters; // Filters to set after dialog is created
	QString currentFilterName;
	int currentFilterIndex;
	bool accepted;
};

#endif // MOONAVOIDANCEDIALOG_HPP

